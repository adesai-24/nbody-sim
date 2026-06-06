#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>
#include "objects/obj_types.h"
#include "objects/vec3.h"
#include "objects/sim_state.h"
#include "barnes_hut/barnes_hut.h"
#include "physics.h"
#include "render.h"

#define G          6.674e-11
#define EPS        1e6
#define DT         3600.0   /* one hour per physics step */
#define SUBSTEPS   24       /* 24 steps/frame -> ~1 day of sim per frame */
#define TRAIL_CAP  2000
#define WIN_W      1280
#define WIN_H      720
#define N          2        // number of bodies in the current system

void init(int n) {
    bodies = malloc(sizeof(Planetoid) * n);
    trails = malloc(sizeof(Trail) * n);
    pool = malloc(sizeof(OctNode) *4*n);
    pool_idx = 0;
}

int main(void) {
    init(N);
    if (!bodies || !trails || !pool) {
        fprintf(stderr, "allocation failed\n");
        free(bodies);
        free(trails);
        free(pool);
        return 1;
    }

    bodies[0] = body_init((Vec3){0,0,0}, (Vec3){0,0,0}, 1.989e30, 696340e3, "Sun");
    bodies[1] = body_init((Vec3){1.496e11,0,0}, (Vec3){0,29783,0}, 5.972e24, 6371e3, "Earth");


    for (int i = 0; i < N; i++) {
        trails[i] = trail_init(TRAIL_CAP);
    }

    printf("initial energy: %.6e J\n", total_energy(bodies, N, G));

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(WIN_W, WIN_H, "nbody - N-body simulator");
    SetTargetFPS(60);
    ui_init();

    SimCamera cam = camera_default();
    long step = 0;

    /* Seed accelerations for the first half-kick of velocity Verlet. While
     * paused no positions change, so this stays valid until we resume. */
    apply_grav(bodies, N, G, EPS);

    while (!WindowShouldClose()) {
        handle_input(&cam);

        if (!cam.paused) {
            for (int s = 0; s < SUBSTEPS; s++) {
                integrate(bodies, N, DT);          // half-kick + drift
                apply_grav(bodies, N, G, EPS);     // forces at new positions
                integrate_finish(bodies, N, DT);   // closing half-kick
                for (int i = 0; i < N; i++)
                    trail_push(&trails[i], bodies[i].pos);
                step++;
            }
        }
        camera_autofit(&cam, bodies, N);

        double E = total_energy(bodies, N, G);
        render_frame(trails, bodies, N, step, E, DT, GetFPS(), cam);
    }

    ui_shutdown();
    CloseWindow();

    for (int i = 0; i < N; i++) {
        body_free(&bodies[i]);
        trail_free(&trails[i]);
    }
    free(pool);
    free(bodies);
    free(trails);
    // free(pool);
    return 0;
}
