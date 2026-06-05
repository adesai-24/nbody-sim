#include "render.h"
#include <raylib.h>
#include <math.h>
#include <stdio.h>

/* ---- tuning constants ------------------------------------------------- */

#define AU_M           1.496e11   /* metres in one astronomical unit       */
#define DEFAULT_AU_PX  300.0      /* on-screen pixels per AU at reset zoom  */

#define SIZE_K         2.2e-9     /* scales cbrt(mass) -> screen radius     */
#define BODY_MIN_PX    3.0f       /* clamp so tiny bodies stay visible      */
#define BODY_MAX_PX    30.0f      /* clamp so the Sun doesn't swallow view  */

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

static float body_screen_radius(const Planetoid *b) {
    double r = cbrt((double)b->mass) * SIZE_K;
    if (r < BODY_MIN_PX) r = BODY_MIN_PX;
    if (r > BODY_MAX_PX) r = BODY_MAX_PX;
    return (float)r;
}

/* ---- camera ----------------------------------------------------------- */

SimCamera camera_default(void) {
    SimCamera c;
    c.scale = DEFAULT_AU_PX / AU_M;
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
        float r = body_screen_radius(p);
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
