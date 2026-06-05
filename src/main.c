#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>
#include "objects/obj_types.h"
#include "body.h"
#include "physics.h"
#include "render.h"

#define G          6.674e-11
#define EPS        1e6
#define DT         3600.0   /* one hour per physics step */
#define SUBSTEPS   24       /* 24 steps/frame -> ~1 day of sim per frame */
#define TRAIL_CAP  2000
#define WIN_W      1280
#define WIN_H      720

int main(void) {
    int n = 2;
    Planetoid *bodies = malloc(sizeof(Planetoid) * n);
    Trail     *trails = malloc(sizeof(Trail) * n);
    if (!bodies || !trails) {
        fprintf(stderr, "allocation failed\n");
        free(bodies);
        free(trails);
        return 1;
    }

    bodies[0] = body_init((Vec3){0,0,0},        (Vec3){0,0,0},     1.989e30, 696340e3, "Sun");
    bodies[1] = body_init((Vec3){1.496e11,0,0}, (Vec3){0,29783,0}, 5.972e24, 6371e3,   "Earth");

    for (int i = 0; i < n; i++) trails[i] = trail_init(TRAIL_CAP);

    printf("initial energy: %.6e J\n", total_energy(bodies, n, G));

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(WIN_W, WIN_H, "nbody - N-body simulator");
    SetTargetFPS(60);

    SimCamera cam = camera_default();
    long step = 0;

    /* Seed accelerations for the first half-kick of velocity Verlet. While
     * paused no positions change, so this stays valid until we resume. */
    apply_grav(bodies, n, G, EPS);

    while (!WindowShouldClose()) {
        handle_input(&cam);

        if (!cam.paused) {
            for (int s = 0; s < SUBSTEPS; s++) {
                integrate(bodies, n, DT);          // half-kick + drift
                apply_grav(bodies, n, G, EPS);     // forces at new positions
                integrate_finish(bodies, n, DT);   // closing half-kick
                for (int i = 0; i < n; i++)
                    trail_push(&trails[i], bodies[i].pos);
                step++;
            }
        }

        camera_autofit(&cam, bodies, n);
        double E = total_energy(bodies, n, G);

        BeginDrawing();
            ClearBackground(BLACK);
            draw_trails(trails, bodies, n, cam);
            draw_bodies(bodies, n, cam);
            draw_hud(bodies, n, step, E, DT, GetFPS());
            draw_scale_legend(cam);
            if (cam.paused)
                DrawText("PAUSED", WIN_W / 2 - 40, 10, 24, YELLOW);
        EndDrawing();
    }

    CloseWindow();

    for (int i = 0; i < n; i++) {
        body_free(&bodies[i]);
        trail_free(&trails[i]);
    }
    free(bodies);
    free(trails);
    return 0;
}
