#include "render.h"
#include <raylib.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>

/* ---- tuning constants ------------------------------------------------- */

#define AU_M           1.496e11   /* metres in one astronomical unit       */
#define DEFAULT_AU_PX  300.0      /* on-screen pixels per AU at reset zoom  */
#define DEFAULT_SCALE  (DEFAULT_AU_PX / AU_M)  /* px per metre at reset zoom */

#define SIZE_K         2.2e-9     /* scales cbrt(mass) -> screen radius     */
#define BODY_MIN_PX    1.5f       /* clamp so tiny/zoomed-out bodies show   */
#define BODY_MAX_PX    64.0f      /* clamp so a body can't fill the screen  */

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
 * shrink as you zoom out and grow as you zoom in, then clamped so they never
 * vanish entirely or swallow the view. */
static float body_screen_radius(const Planetoid *b, double scale) {
    double zoom = scale / DEFAULT_SCALE;          /* 1.0 at reset zoom */
    double r = cbrt((double)b->mass) * SIZE_K * zoom;
    if (r < BODY_MIN_PX) r = BODY_MIN_PX;
    if (r > BODY_MAX_PX) r = BODY_MAX_PX;
    return (float)r;
}

/* ---- UI theme & fonts ------------------------------------------------- */

#define UI_FONT_BASE   64         /* glyph atlas size; downscaled when drawn */

static Font ui_font;              /* regular weight: labels and values       */
static Font ui_font_bold;         /* bold weight: title and pause pill       */
static bool ui_font_loaded      = false;
static bool ui_font_bold_loaded = false;

/* Quiet, low-contrast palette so the overlay sits lightly over the sim. */
static const Color UI_PANEL  = { 14,  16,  22, 170 };   /* translucent slab  */
static const Color UI_LABEL  = {132, 142, 158, 255 };   /* dim row labels    */
static const Color UI_VALUE  = {226, 232, 240, 255 };   /* bright values     */
static const Color UI_ACCENT = {125, 211, 252, 255 };   /* soft cyan accent  */
static const Color UI_MUTED  = {120, 130, 146, 220 };   /* hints, scale bar  */

void ui_init(void) {
    const char *reg  = "assets/fonts/GeistMono-Regular.ttf";
    const char *bold = "assets/fonts/GeistMono-Bold.ttf";

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
        ui_font_bold = ui_font;   /* share the regular face (don't unload) */
    }
}

void ui_shutdown(void) {
    if (ui_font_loaded)      UnloadFont(ui_font);
    if (ui_font_bold_loaded) UnloadFont(ui_font_bold);
    ui_font_loaded = ui_font_bold_loaded = false;
}

/* Draw text in the UI font; spacing scales with size for even tracking. */
static void ui_text(Font f, const char *s, float x, float y, float size, Color c) {
    DrawTextEx(f, s, (Vector2){x, y}, size, size * 0.06f, c);
}
static float ui_text_w(Font f, const char *s, float size) {
    return MeasureTextEx(f, s, size, size * 0.06f).x;
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

/* Auto fit-to-view: each frame, rescale and re-center so every body stays in
 * frame as the system expands or collapses. The target view (scale + offsets
 * that frame the bodies' bounding box) is approached with a per-frame lerp so
 * the zoom glides instead of snapping. A manual scroll/drag clears auto_fit;
 * F toggles it and R re-enables it. */
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

    /* Place the bounding-box center at the window center (screen +y is down,
     * world +y is up, so the y offset adds rather than subtracts). */
    double target_off_x = w / 2.0 - cx * target_scale;
    double target_off_y = h / 2.0 + cy * target_scale;

    cam->scale += (target_scale - cam->scale) * FIT_SMOOTH;
    cam->off_x += (target_off_x - cam->off_x) * FIT_SMOOTH;
    cam->off_y += (target_off_y - cam->off_y) * FIT_SMOOTH;
}

void handle_input(SimCamera *cam) {
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        /* Zoom about the cursor: keep the world point under the mouse fixed. */
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

    if (IsKeyPressed(KEY_F)) {
        cam->auto_fit = !cam->auto_fit;
    }

    if (IsKeyPressed(KEY_R)) {
        int was_paused = cam->paused;
        *cam = camera_default();
        cam->paused = was_paused;   /* reset the view, not the run state */
    }

    if (IsKeyPressed(KEY_SPACE)) {
        cam->paused = !cam->paused;
    }
}

/* ---- drawing ---------------------------------------------------------- */

void draw_trails(Trail *trails, Planetoid *bodies, int n, SimCamera cam) {
    (void)bodies;
    for (int i = 0; i < n; i++) {
        Trail *t = &trails[i];
        if (t->count < 2) continue;

        Color base = PALETTE[i % PALETTE_N];
        int cap = t->capacity;

        /* Walk oldest -> newest, fading alpha from ~0 up to 1 at the head. */
        for (int k = 0; k < t->count - 1; k++) {
            int idx_a = ((t->head - t->count + 1 + k) % cap + cap) % cap;
            int idx_b = (idx_a + 1) % cap;

            Vector2 a = world_to_screen(cam, t->points[idx_a]);
            Vector2 b = world_to_screen(cam, t->points[idx_b]);

            float alpha = (float)(k + 1) / (float)(t->count - 1);
            DrawLineV(a, b, Fade(base, alpha));
        }
    }
}

void draw_bodies(Planetoid *bodies, int n, SimCamera cam) {
    for (int i = 0; i < n; i++) {
        Planetoid *p = &bodies[i];
        Vector2 c = world_to_screen(cam, p->pos);
        float r = body_screen_radius(p, cam.scale);
        Color col = PALETTE[i % PALETTE_N];

        DrawCircleV(c, r, col);
        if (p->name) {
            ui_text(ui_font, p->name, c.x + r + 5.0f, c.y - 7.0f, 14.0f,
                    Fade(RAYWHITE, 0.85f));
        }
    }
}

void draw_hud(Planetoid *bodies, int n, long step, double energy, double dt,
              int fps, SimCamera cam) {
    (void)bodies;

    const float px = 18.0f, py = 18.0f;   /* panel top-left           */
    const float pad = 14.0f;
    const float pw  = 212.0f;             /* panel width              */

    const float title_sz = 22.0f;
    const float row_sz   = 15.0f;
    const float row_h    = 23.0f;
    const float label_x  = px + pad;
    const float value_x  = px + pad + 80.0f;

    const int   rows = 5;
    const float ph = pad + title_sz + 12.0f + rows * row_h + pad - 8.0f;

    /* translucent slab */
    DrawRectangleRounded((Rectangle){px, py, pw, ph}, 0.10f, 8, UI_PANEL);

    float ty = py + pad;

    /* title + right-aligned view mode (AUTO / MANUAL) */
    ui_text(ui_font_bold, "nbody", label_x, ty, title_sz, UI_ACCENT);
    const char *mode = cam.auto_fit ? "AUTO" : "MANUAL";
    float mw = ui_text_w(ui_font, mode, 11.0f);
    ui_text(ui_font, mode, px + pw - pad - mw, ty + 7.0f, 11.0f, UI_MUTED);

    /* hairline accent under the title */
    ty += title_sz + 6.0f;
    DrawRectangle((int)label_x, (int)ty, (int)(pw - 2 * pad), 1, Fade(UI_ACCENT, 0.35f));
    ty += 6.0f;

    #define ROW(lbl, ...)                                                      \
        do {                                                                   \
            ui_text(ui_font, lbl, label_x, ty, row_sz, UI_LABEL);              \
            ui_text(ui_font, TextFormat(__VA_ARGS__), value_x, ty, row_sz,     \
                    UI_VALUE);                                                 \
            ty += row_h;                                                       \
        } while (0)

    ROW("step",   "%ld", step);
    ROW("bodies", "%d", n);
    ROW("energy", "%.3e", energy);
    ROW("dt",     "%.0f s", dt);
    ROW("fps",    "%d", fps);
    #undef ROW

    /* centered pause pill */
    if (cam.paused) {
        const char *s = "PAUSED";
        const float sz = 15.0f;
        float w = ui_text_w(ui_font_bold, s, sz);
        float bx = (GetScreenWidth() - w) / 2.0f;
        DrawRectangleRounded((Rectangle){bx - 14.0f, 14.0f, w + 28.0f, sz + 14.0f},
                             0.5f, 8, UI_PANEL);
        ui_text(ui_font_bold, s, bx, 21.0f, sz, UI_ACCENT);
    }

    /* controls hint, bottom-left */
    ui_text(ui_font, "scroll zoom    drag pan    F auto-fit    R reset    space pause",
            18.0f, GetScreenHeight() - 26.0f, 13.0f, UI_MUTED);
}

void draw_scale_bar(SimCamera cam) {
    if (cam.scale <= 0.0) return;

    const double target_px = 170.0;            /* aim for a bar about this wide */
    double dist_m = target_px / cam.scale;     /* metres spanned by target_px   */

    /* Choose the unit first, then round to a clean 1/2/5 x 10^k *in that unit*
     * so the printed number reads nicely (e.g. "2 AU", "500 km"). */
    const char *unit;
    double unit_m;                             /* metres per unit */
    if (dist_m >= 0.01 * AU_M) { unit = "AU"; unit_m = AU_M;   }
    else                       { unit = "km"; unit_m = 1000.0; }

    double dist_u = dist_m / unit_m;
    double e    = floor(log10(dist_u));
    double base = pow(10.0, e);
    double frac = dist_u / base;               /* in [1, 10) */
    double nice = (frac >= 5.0) ? 5.0 : (frac >= 2.0) ? 2.0 : 1.0;
    double span_u = nice * base;               /* round distance in display unit */
    float  bar    = (float)(span_u * unit_m * cam.scale);

    char label[48];
    snprintf(label, sizeof(label), "%g %s", span_u, unit);

    int sw = GetScreenWidth(), sh = GetScreenHeight();
    float x2 = sw - 28.0f, x1 = x2 - bar, yb = sh - 30.0f;
    Color c = UI_MUTED;

    DrawLineEx((Vector2){x1, yb},        (Vector2){x2, yb},        1.5f, c);
    DrawLineEx((Vector2){x1, yb - 5.0f}, (Vector2){x1, yb + 5.0f}, 1.5f, c);
    DrawLineEx((Vector2){x2, yb - 5.0f}, (Vector2){x2, yb + 5.0f}, 1.5f, c);

    float tw = ui_text_w(ui_font, label, 13.0f);
    ui_text(ui_font, label, x2 - tw, yb - 19.0f, 13.0f, c);
}
