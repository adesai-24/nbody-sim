#include "render.h"
#include <raylib.h>
#include <math.h>
#include <stdio.h>

/* ---- tuning constants ------------------------------------------------- */

#define AU_M           1.496e11   /* metres in one astronomical unit       */
#define DEFAULT_AU_PX  300.0      /* on-screen pixels per AU at reset zoom  */
#define DEFAULT_SCALE  (DEFAULT_AU_PX / AU_M)  /* pixels per metre at reset */

#define SIZE_K         2.2e-9     /* scales cbrt(mass) -> screen radius     */
#define BODY_MIN_PX    2.0f       /* floor so distant bodies stay visible   */

#define ZOOM_STEP      1.1        /* zoom multiplier per wheel notch        */

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

/* Screen radius scales with the zoom level so bodies grow when you zoom in
 * and shrink when you zoom out (relative to the default zoom). cbrt(mass)
 * keeps the bodies in sensible proportion to one another; a small floor
 * stops far-out bodies from disappearing entirely. No upper cap, so zooming
 * in keeps revealing a body's true relative size. */
static float body_screen_radius(const Planetoid *b, SimCamera cam) {
    double zoom = cam.scale / DEFAULT_SCALE;
    double r = cbrt((double)b->mass) * SIZE_K * zoom;
    if (r < BODY_MIN_PX) r = BODY_MIN_PX;
    return (float)r;
}

/* ---- camera ----------------------------------------------------------- */

SimCamera camera_default(void) {
    SimCamera c;
    c.scale = DEFAULT_SCALE;
    c.off_x = GetScreenWidth() / 2.0;
    c.off_y = GetScreenHeight() / 2.0;
    c.paused = 0;
    return c;
}

void handle_input(SimCamera *cam) {
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        /* Zoom about the cursor: keep the world point under the mouse fixed. */
        Vector2 m = GetMousePosition();
        double f = pow(ZOOM_STEP, wheel);
        cam->off_x = m.x - (m.x - cam->off_x) * f;
        cam->off_y = m.y - (m.y - cam->off_y) * f;
        cam->scale *= f;
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 d = GetMouseDelta();
        cam->off_x += d.x;
        cam->off_y += d.y;
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
        float r = body_screen_radius(p, cam);
        Color col = PALETTE[i % PALETTE_N];

        DrawCircleV(c, r, col);
        if (p->name) {
            DrawText(p->name, (int)(c.x + r + 4), (int)(c.y - 6), 12, LIGHTGRAY);
        }
    }
}

void draw_hud(Planetoid *bodies, int n, long step, double energy, double dt, int fps) {
    (void)bodies;
    char buf[128];
    int y = 10;
    const int line = 20;
    const Color fg = RAYWHITE;

    DrawText("nbody", 10, y, 20, GOLD); y += line + 4;

    snprintf(buf, sizeof(buf), "step:   %ld", step);
    DrawText(buf, 10, y, 18, fg); y += line;

    snprintf(buf, sizeof(buf), "bodies: %d", n);
    DrawText(buf, 10, y, 18, fg); y += line;

    snprintf(buf, sizeof(buf), "energy: %.6e", energy);
    DrawText(buf, 10, y, 18, fg); y += line;

    snprintf(buf, sizeof(buf), "dt:     %.0f s", dt);
    DrawText(buf, 10, y, 18, fg); y += line;

    snprintf(buf, sizeof(buf), "fps:    %d", fps);
    DrawText(buf, 10, y, 18, fg); y += line;

    DrawText("[scroll] zoom  [drag] pan  [R] reset  [space] pause",
             10, GetScreenHeight() - 24, 16, GRAY);
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

/* Bottom-right reference bar. Its on-screen length always sits near a target
 * width, but it is labelled with a round real-world distance whose unit adapts
 * to the zoom (m -> km -> AU -> ly), so it stays meaningful from planet radii
 * out to interstellar distances. */
void draw_scale_legend(SimCamera cam) {
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

    int W = GetScreenWidth();
    int H = GetScreenHeight();
    const int margin = 24;
    float x1 = W - margin;
    float x0 = x1 - bar_px;
    float y  = H - margin;
    const float tick = 6.0f;
    const Color c = RAYWHITE;

    DrawLineEx((Vector2){x0, y}, (Vector2){x1, y}, 2.0f, c);          /* bar  */
    DrawLineEx((Vector2){x0, y - tick}, (Vector2){x0, y + tick}, 2.0f, c); /* ends */
    DrawLineEx((Vector2){x1, y - tick}, (Vector2){x1, y + tick}, 2.0f, c);

    char label[64];
    if (value >= 1.0) {
        char num[40];
        group_commas((long long)llround(value), num, sizeof(num));
        snprintf(label, sizeof(label), "%s %s", num, SCALE_UNITS[u].name);
    } else {
        snprintf(label, sizeof(label), "%g %s", value, SCALE_UNITS[u].name);
    }
    int tw = MeasureText(label, 16);
    DrawText(label, (int)((x0 + x1) / 2 - tw / 2), (int)(y - 22), 16, c);
}
