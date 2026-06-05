#include "render.h"
#include <raylib.h>
#include <rlgl.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>

/* ---- tuning constants ------------------------------------------------- */

#define AU_M           1.496e11   /* metres in one astronomical unit       */
#define DEFAULT_AU_PX  300.0      /* on-screen pixels per AU at reset zoom  */
#define DEFAULT_SCALE  (DEFAULT_AU_PX / AU_M)  /* px per metre at reset zoom */

#define SIZE_K         2.2e-9     /* scales cbrt(mass) -> screen radius     */
#define BODY_MIN_PX    2.0f       /* floor so distant bodies stay visible   */

#define ZOOM_STEP      1.1        /* zoom multiplier per wheel notch        */

#define FIT_MARGIN     0.85       /* fraction of the window the bodies fill  */
#define FIT_SMOOTH     0.10       /* per-frame lerp toward the fitted view   */
#define FIT_MIN_SPAN   (0.2 * AU_M) /* floor on span so a single/clustered
                                      set of bodies doesn't zoom to infinity */

/* Bodies are colored by index, cycling through this palette. */
static const Color PALETTE[] = {
    GOLD, SKYBLUE, RED, GREEN, ORANGE,
    PURPLE, BEIGE, LIME, PINK, VIOLET,
};
static const int PALETTE_N = (int)(sizeof(PALETTE) / sizeof(PALETTE[0]));

/* ---- glassmorphism theme ---------------------------------------------- */

static const Color BG_TOP   = { 12,  16,  26, 255 };  /* deep navy, top     */
static const Color BG_BOT   = {  3,   4,   8, 255 };  /* near-black, bottom */

static const Color UI_TITLE = {150, 214, 255, 255 };  /* cyan accent        */
static const Color UI_LABEL = {150, 162, 184, 255 };  /* dim row labels     */
static const Color UI_VALUE = {233, 240, 250, 255 };  /* bright values      */
static const Color UI_MUTED = {130, 142, 165, 210 };  /* hints, scale bar   */

/* Glass fill + edge, as 0..1 RGBA passed to the shader. The alpha channel is
 * a blend *strength*, not opacity: how strongly the frosted tint tints the
 * blurred backdrop behind the card. */
static const float GLASS_TINT[4]   = { 0.80f, 0.86f, 0.96f, 0.13f };
static const float GLASS_BORDER[4] = { 0.90f, 0.95f, 1.00f, 0.35f };

/* ---- coordinate / sizing helpers -------------------------------------- */

/* World (metres) -> screen (pixels). World +y points up, so the screen y
 * axis is flipped relative to the simulation. */
static Vector2 world_to_screen(SimCamera cam, Vec3 p) {
    Vector2 s;
    s.x = (float)(cam.off_x + p.x * cam.scale);
    s.y = (float)(cam.off_y - p.y * cam.scale);
    return s;
}

/* Apparent body radius in pixels. Sized by the cube root of mass (a 1000x
 * heavier body looks ~10x wider) and scaled with the zoom level, so bodies
 * shrink as you zoom out and grow as you zoom in. A small floor keeps distant
 * bodies visible, and the lack of an upper cap preserves relative size when
 * you zoom in close. */
static float body_screen_radius(const Planetoid *b, SimCamera cam) {
    double zoom = cam.scale / DEFAULT_SCALE;          /* 1.0 at reset zoom */
    double r = cbrt((double)b->mass) * SIZE_K * zoom;
    if (r < BODY_MIN_PX) r = BODY_MIN_PX;
    return (float)r;
}

/* ---- fonts, shader, render targets ------------------------------------ */

#define UI_FONT_BASE 64           /* glyph atlas size; downscaled when drawn */

static Font ui_font;              /* Outfit Regular: labels and values       */
static Font ui_font_bold;         /* Outfit Bold: title and pause pill       */
static bool ui_font_loaded      = false;
static bool ui_font_bold_loaded = false;

/* Frosted-glass panel shader: samples the blurred scene as a backdrop, masks
 * it to a rounded rectangle, and adds the tint + edge highlight. */
static Shader glass;
static bool   glass_loaded = false;
static int    loc_res, loc_min, loc_max, loc_radius, loc_borderw, loc_tint, loc_border;

/* Offscreen targets: the crisp scene, then two progressively smaller buffers
 * used to build a cheap separable-ish blur of it. */
static RenderTexture2D scene_rt, blur_a, blur_b;
static int rt_w = 0, rt_h = 0;

static const char *GLASS_FS =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "out vec4 finalColor;\n"
    "uniform sampler2D texture0;\n"   /* blurred scene, screen-aligned 0..1 */
    "uniform vec2  uResolution;\n"
    "uniform vec2  uRectMin;\n"
    "uniform vec2  uRectMax;\n"
    "uniform float uRadius;\n"
    "uniform float uBorderW;\n"
    "uniform vec4  uTint;\n"
    "uniform vec4  uBorder;\n"
    "float sdRound(vec2 p, vec2 b, float r){\n"
    "    vec2 q = abs(p) - b + vec2(r);\n"
    "    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;\n"
    "}\n"
    "void main(){\n"
    /* gl_FragCoord is bottom-up window space; flip y to match raylib's
       top-left screen coords used for the rect bounds. */
    "    vec2 px = vec2(gl_FragCoord.x, uResolution.y - gl_FragCoord.y);\n"
    "    vec2 c  = 0.5 * (uRectMin + uRectMax);\n"
    "    vec2 b  = 0.5 * (uRectMax - uRectMin);\n"
    "    float d = sdRound(px - c, b, uRadius);\n"
    "    float aa = fwidth(d) + 0.0001;\n"
    "    float inside = 1.0 - smoothstep(0.0, aa, d);\n"
    "    if (inside <= 0.001) discard;\n"
    "    vec3 backdrop = texture(texture0, fragTexCoord).rgb;\n"
    "    vec3 col = mix(backdrop, uTint.rgb, uTint.a);\n"
    "    float t = clamp((px.y - uRectMin.y) / max(uRectMax.y - uRectMin.y, 1.0), 0.0, 1.0);\n"
    "    col += (1.0 - t) * 0.05;\n"                 /* soft top-down sheen */
    "    float band = smoothstep(uBorderW + 1.0, uBorderW - 0.5, -d);\n"
    "    col = mix(col, uBorder.rgb, band * uBorder.a);\n"
    "    finalColor = vec4(col, inside);\n"
    "}\n";

void ui_init(void) {
    const char *reg  = "assets/fonts/Outfit-Regular.ttf";
    const char *bold = "assets/fonts/Outfit-Bold.ttf";

    if (FileExists(reg)) {
        ui_font = LoadFontEx(reg, UI_FONT_BASE, NULL, 0);
        ui_font_loaded = true;
        SetTextureFilter(ui_font.texture, TEXTURE_FILTER_BILINEAR);
    } else {
        ui_font = GetFontDefault();
    }
    if (FileExists(bold)) {
        ui_font_bold = LoadFontEx(bold, UI_FONT_BASE, NULL, 0);
        ui_font_bold_loaded = true;
        SetTextureFilter(ui_font_bold.texture, TEXTURE_FILTER_BILINEAR);
    } else {
        ui_font_bold = ui_font;
    }

    glass = LoadShaderFromMemory(NULL, GLASS_FS);
    glass_loaded = (glass.id != 0) && (glass.id != rlGetShaderIdDefault());
    if (glass_loaded) {
        loc_res     = GetShaderLocation(glass, "uResolution");
        loc_min     = GetShaderLocation(glass, "uRectMin");
        loc_max     = GetShaderLocation(glass, "uRectMax");
        loc_radius  = GetShaderLocation(glass, "uRadius");
        loc_borderw = GetShaderLocation(glass, "uBorderW");
        loc_tint    = GetShaderLocation(glass, "uTint");
        loc_border  = GetShaderLocation(glass, "uBorder");
    }
}

void ui_shutdown(void) {
    if (ui_font_loaded)      UnloadFont(ui_font);
    if (ui_font_bold_loaded) UnloadFont(ui_font_bold);
    if (glass_loaded)        UnloadShader(glass);
    if (rt_w) {
        UnloadRenderTexture(scene_rt);
        UnloadRenderTexture(blur_a);
        UnloadRenderTexture(blur_b);
    }
    ui_font_loaded = ui_font_bold_loaded = glass_loaded = false;
    rt_w = rt_h = 0;
}

/* (Re)create the offscreen targets if the window size changed. */
static void ensure_targets(void) {
    int w = GetScreenWidth(), h = GetScreenHeight();
    if (w == rt_w && h == rt_h) return;
    if (rt_w) {
        UnloadRenderTexture(scene_rt);
        UnloadRenderTexture(blur_a);
        UnloadRenderTexture(blur_b);
    }
    scene_rt = LoadRenderTexture(w, h);
    blur_a   = LoadRenderTexture((w + 3) / 4, (h + 3) / 4);
    blur_b   = LoadRenderTexture((w + 7) / 8, (h + 7) / 8);
    SetTextureFilter(scene_rt.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(blur_a.texture,   TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(blur_b.texture,   TEXTURE_FILTER_BILINEAR);
    rt_w = w;
    rt_h = h;
}

/* ---- text helpers ----------------------------------------------------- */

static void ui_text(Font f, const char *s, float x, float y, float size, Color c) {
    DrawTextEx(f, s, (Vector2){x, y}, size, 0.0f, c);
}
static float ui_text_w(Font f, const char *s, float size) {
    return MeasureTextEx(f, s, size, 0.0f).x;
}
static void ui_text_right(Font f, const char *s, float xr, float y, float size, Color c) {
    DrawTextEx(f, s, (Vector2){xr - ui_text_w(f, s, size), y}, size, 0.0f, c);
}

/* ---- camera ----------------------------------------------------------- */

SimCamera camera_default(void) {
    SimCamera c;
    c.scale = DEFAULT_AU_PX / AU_M;
    c.off_x = GetScreenWidth() / 2.0;
    c.off_y = GetScreenHeight() / 2.0;
    c.paused = 0;
    c.auto_fit = 1;
    return c;
}

void camera_autofit(SimCamera *cam, const Planetoid *bodies, int n) {
    if (!cam->auto_fit || n <= 0) return;

    double min_x, max_x, min_y, max_y;
    min_x = max_x = bodies[0].pos.x;
    min_y = max_y = bodies[0].pos.y;
    for (int i = 1; i < n; i++) {
        double x = bodies[i].pos.x, y = bodies[i].pos.y;
        if (x < min_x) min_x = x;
        if (x > max_x) max_x = x;
        if (y < min_y) min_y = y;
        if (y > max_y) max_y = y;
    }

    double cx = 0.5 * (min_x + max_x);
    double cy = 0.5 * (min_y + max_y);
    double span_x = max_x - min_x;
    double span_y = max_y - min_y;
    if (span_x < FIT_MIN_SPAN) span_x = FIT_MIN_SPAN;
    if (span_y < FIT_MIN_SPAN) span_y = FIT_MIN_SPAN;

    int w = GetScreenWidth(), h = GetScreenHeight();
    double scale_x = (w * FIT_MARGIN) / span_x;
    double scale_y = (h * FIT_MARGIN) / span_y;
    double target_scale = scale_x < scale_y ? scale_x : scale_y;

    double target_off_x = w / 2.0 - cx * target_scale;
    double target_off_y = h / 2.0 + cy * target_scale;

    cam->scale += (target_scale - cam->scale) * FIT_SMOOTH;
    cam->off_x += (target_off_x - cam->off_x) * FIT_SMOOTH;
    cam->off_y += (target_off_y - cam->off_y) * FIT_SMOOTH;
}

void handle_input(SimCamera *cam) {
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        cam->auto_fit = 0;   /* manual zoom takes over from auto-fit */
        Vector2 m = GetMousePosition();
        double f = pow(ZOOM_STEP, wheel);
        cam->off_x = m.x - (m.x - cam->off_x) * f;
        cam->off_y = m.y - (m.y - cam->off_y) * f;
        cam->scale *= f;
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        cam->auto_fit = 0;   /* manual pan takes over from auto-fit */
        Vector2 d = GetMouseDelta();
        cam->off_x += d.x;
        cam->off_y += d.y;
    }

    if (IsKeyPressed(KEY_F)) cam->auto_fit = !cam->auto_fit;

    if (IsKeyPressed(KEY_R)) {
        int was_paused = cam->paused;
        *cam = camera_default();
        cam->paused = was_paused;   /* reset the view, not the run state */
    }

    if (IsKeyPressed(KEY_SPACE)) cam->paused = !cam->paused;
}

/* ---- scene drawing ---------------------------------------------------- */

static void draw_trails(Trail *trails, int n, SimCamera cam) {
    for (int i = 0; i < n; i++) {
        Trail *t = &trails[i];
        if (t->count < 2) continue;

        Color base = PALETTE[i % PALETTE_N];
        int cap = t->capacity;

        for (int k = 0; k < t->count - 1; k++) {
            int idx_a = ((t->head - t->count + 1 + k) % cap + cap) % cap;
            int idx_b = (idx_a + 1) % cap;

            Vector2 a = world_to_screen(cam, t->points[idx_a]);
            Vector2 b = world_to_screen(cam, t->points[idx_b]);

            float alpha = (float)(k + 1) / (float)(t->count - 1);
            DrawLineEx(a, b, 1.5f, Fade(base, alpha));
        }
    }
}

static void draw_bodies(Planetoid *bodies, int n, SimCamera cam) {
    for (int i = 0; i < n; i++) {
        Planetoid *p = &bodies[i];
        Vector2 c = world_to_screen(cam, p->pos);
        float r = body_screen_radius(p, cam);
        Color col = PALETTE[i % PALETTE_N];

        DrawCircleV(c, r * 1.9f, Fade(col, 0.12f));   /* soft glow halo */
        DrawCircleV(c, r, col);
        if (p->name)
            ui_text(ui_font, p->name, c.x + r + 6.0f, c.y - 8.0f, 15.0f,
                    Fade(RAYWHITE, 0.88f));
    }
}

/* ---- glass HUD -------------------------------------------------------- */

/* Composite one frosted-glass card over the (already drawn) scene by sampling
 * the blurred scene through the rounded-rect glass shader. */
static void glass_panel(Rectangle r, float radius) {
    /* soft drop shadow: a few stacked translucent rounded rects */
    for (int i = 3; i >= 1; i--) {
        float g = i * 4.0f;
        Rectangle s = { r.x - g, r.y - g + 4.0f, r.width + 2 * g, r.height + 2 * g };
        DrawRectangleRounded(s, 0.18f, 8, Fade(BLACK, 0.06f));
    }

    if (!glass_loaded) {                       /* fallback: plain translucent slab */
        float roundness = 2.0f * radius / fminf(r.width, r.height);
        DrawRectangleRounded(r, roundness, 8, (Color){18, 22, 32, 180});
        return;
    }

    float res[2]    = { (float)GetScreenWidth(), (float)GetScreenHeight() };
    float rmin[2]   = { r.x, r.y };
    float rmax[2]   = { r.x + r.width, r.y + r.height };
    float radf      = radius;
    float bw        = 1.5f;

    BeginShaderMode(glass);
        SetShaderValue(glass, loc_res,     res,  SHADER_UNIFORM_VEC2);
        SetShaderValue(glass, loc_min,     rmin, SHADER_UNIFORM_VEC2);
        SetShaderValue(glass, loc_max,     rmax, SHADER_UNIFORM_VEC2);
        SetShaderValue(glass, loc_radius,  &radf, SHADER_UNIFORM_FLOAT);
        SetShaderValue(glass, loc_borderw, &bw,   SHADER_UNIFORM_FLOAT);
        SetShaderValue(glass, loc_tint,    GLASS_TINT,   SHADER_UNIFORM_VEC4);
        SetShaderValue(glass, loc_border,  GLASS_BORDER, SHADER_UNIFORM_VEC4);
        /* draw the blurred scene stretched to full screen (flipped upright);
         * the shader keeps only the rounded card region. */
        DrawTexturePro(blur_b.texture,
            (Rectangle){0, 0, (float)blur_b.texture.width, -(float)blur_b.texture.height},
            (Rectangle){0, 0, res[0], res[1]}, (Vector2){0, 0}, 0.0f, WHITE);
    EndShaderMode();
}

/* Distance units the scale bar climbs through as you zoom out, smallest
 * first. Each entry is how many metres are in one of that unit. */
typedef struct { const char *name; double metres; } ScaleUnit;
static const ScaleUnit SCALE_UNITS[] = {
    { "m",  1.0 },
    { "km", 1.0e3 },
    { "AU", 1.496e11 },
    { "ly", 9.4607e15 },
};
static const int SCALE_UNITS_N = (int)(sizeof(SCALE_UNITS) / sizeof(SCALE_UNITS[0]));

/* Snap a positive value to the nearest "nice" 1/2/5 x 10^k below it. */
static double nice_round(double v) {
    double e = floor(log10(v));
    double base = pow(10.0, e);
    double f = v / base;            /* 1 <= f < 10 */
    double nf = (f < 2.0) ? 1.0 : (f < 5.0) ? 2.0 : 5.0;
    return nf * base;
}

/* Write a non-negative integer with thousands separators, e.g. 100000000 ->
 * "100,000,000", so large round distances read cleanly instead of in
 * scientific notation. */
static void group_commas(long long v, char *out, size_t n) {
    char tmp[32];
    int len = snprintf(tmp, sizeof(tmp), "%lld", v);
    int commas = (len - 1) / 3;
    int outlen = len + commas;
    if (len < 0 || (size_t)outlen >= n) { snprintf(out, n, "%lld", v); return; }
    out[outlen] = '\0';
    int oi = outlen - 1, cnt = 0;
    for (int i = len - 1; i >= 0; i--) {
        out[oi--] = tmp[i];
        if (++cnt % 3 == 0 && i > 0) out[oi--] = ',';
    }
}

static void draw_scale_bar(SimCamera cam) {
    if (cam.scale <= 0.0) return;

    const double target_px = 150.0;          /* desired bar length          */
    double metres_per_px = 1.0 / cam.scale;
    double raw_m = target_px * metres_per_px; /* metres spanned by target_px */

    /* Pick the largest unit in which the bar is at least one unit long. */
    int u = 0;
    for (int i = 0; i < SCALE_UNITS_N; i++) {
        if (raw_m >= SCALE_UNITS[i].metres) u = i;
    }
    double value = nice_round(raw_m / SCALE_UNITS[u].metres);
    double bar_m = value * SCALE_UNITS[u].metres;
    float  bar_px = (float)(bar_m * cam.scale);

    int sw = GetScreenWidth(), sh = GetScreenHeight();
    const int margin = 24;
    float x1 = sw - margin;
    float x0 = x1 - bar_px;
    float y  = sh - margin;
    const float tick = 6.0f;
    const Color c = UI_MUTED;

    DrawLineEx((Vector2){x0, y}, (Vector2){x1, y}, 1.5f, c);
    DrawLineEx((Vector2){x0, y - tick}, (Vector2){x0, y + tick}, 1.5f, c);
    DrawLineEx((Vector2){x1, y - tick}, (Vector2){x1, y + tick}, 1.5f, c);

    char label[64];
    if (value >= 1.0) {
        char num[40];
        group_commas((long long)llround(value), num, sizeof(num));
        snprintf(label, sizeof(label), "%s %s", num, SCALE_UNITS[u].name);
    } else {
        snprintf(label, sizeof(label), "%g %s", value, SCALE_UNITS[u].name);
    }
    ui_text_right(ui_font, label, x1, y - 20.0f, 14.0f, c);
}

static void draw_hud(int n, long step, double energy, double dt, int fps, SimCamera cam) {
    const float x = 22.0f, y = 22.0f, pad = 18.0f, w = 244.0f;
    const float title_sz = 24.0f, row_sz = 15.0f, row_h = 26.0f;

    const int   rows = 5;
    const float h = pad + title_sz + 14.0f + rows * row_h + pad - 8.0f;

    glass_panel((Rectangle){x, y, w, h}, 18.0f);

    float tx = x + pad, ty = y + pad;

    ui_text(ui_font_bold, "nbody", tx, ty, title_sz, UI_TITLE);
    const char *mode = cam.auto_fit ? "AUTO" : "MANUAL";
    ui_text_right(ui_font, mode, x + w - pad, ty + 8.0f, 11.0f, UI_MUTED);

    ty += title_sz + 8.0f;
    DrawRectangle((int)tx, (int)ty, (int)(w - 2 * pad), 1, Fade(UI_TITLE, 0.30f));
    ty += 8.0f;

    #define ROW(lbl, ...)                                                       \
        do {                                                                    \
            ui_text(ui_font, lbl, tx, ty, row_sz, UI_LABEL);                    \
            ui_text_right(ui_font, TextFormat(__VA_ARGS__), x + w - pad, ty,    \
                          row_sz, UI_VALUE);                                    \
            ty += row_h;                                                        \
        } while (0)

    ROW("step",   "%ld", step);
    ROW("bodies", "%d", n);
    ROW("energy", "%.3e", energy);
    ROW("dt",     "%.0f s", dt);
    ROW("fps",    "%d", fps);
    #undef ROW

    if (cam.paused) {
        const char *s = "PAUSED";
        const float sz = 15.0f;
        float pw = ui_text_w(ui_font_bold, s, sz);
        float bx = (GetScreenWidth() - pw) / 2.0f;
        glass_panel((Rectangle){bx - 18.0f, 16.0f, pw + 36.0f, sz + 18.0f}, 13.0f);
        ui_text(ui_font_bold, s, bx, 24.0f, sz, UI_TITLE);
    }

    ui_text(ui_font, "scroll  zoom      drag  pan      F  auto-fit      R  reset      space  pause",
            22.0f, GetScreenHeight() - 28.0f, 13.0f, UI_MUTED);
}

/* ---- frame orchestration ---------------------------------------------- */

void render_frame(Trail *trails, Planetoid *bodies, int n,
                  long step, double energy, double dt, int fps, SimCamera cam) {
    ensure_targets();

    /* 1. draw the crisp scene into its own buffer */
    BeginTextureMode(scene_rt);
        DrawRectangleGradientV(0, 0, rt_w, rt_h, BG_TOP, BG_BOT);
        draw_trails(trails, n, cam);
        draw_bodies(bodies, n, cam);
    EndTextureMode();

    /* 2. downsample it twice to build a soft blur (orientation preserved) */
    BeginTextureMode(blur_a);
        DrawTexturePro(scene_rt.texture,
            (Rectangle){0, 0, (float)scene_rt.texture.width, (float)scene_rt.texture.height},
            (Rectangle){0, 0, (float)blur_a.texture.width, (float)blur_a.texture.height},
            (Vector2){0, 0}, 0.0f, WHITE);
    EndTextureMode();
    BeginTextureMode(blur_b);
        DrawTexturePro(blur_a.texture,
            (Rectangle){0, 0, (float)blur_a.texture.width, (float)blur_a.texture.height},
            (Rectangle){0, 0, (float)blur_b.texture.width, (float)blur_b.texture.height},
            (Vector2){0, 0}, 0.0f, WHITE);
    EndTextureMode();

    /* 3. present: crisp scene, then the frosted-glass HUD on top */
    BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(scene_rt.texture,
            (Rectangle){0, 0, (float)rt_w, -(float)rt_h},
            (Rectangle){0, 0, (float)rt_w, (float)rt_h},
            (Vector2){0, 0}, 0.0f, WHITE);
        draw_hud(n, step, energy, dt, fps, cam);
        draw_scale_bar(cam);
    EndDrawing();
}
