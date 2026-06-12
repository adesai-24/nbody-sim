#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <raylib.h>
#include "objects/obj_types.h"
#include "body.h"
#include "physics.h"
#include "render.h"
#include "scenarios.h"

#define G          6.674e-11
#define EPS        1e6
#define DT         3600.0   /* one hour per physics step */
#define SUBSTEPS   24       /* 24 steps/frame -> ~1 day of sim per frame */
#define TRAIL_CAP  2000

#define WIN_W      1280
#define WIN_H      720

static void unload_sim(Planetoid *bodies, Trail *trails, int n) {
    for (int i = 0; i < n; i++) {
        body_free(&bodies[i]);
        trail_free(&trails[i]);
    }
}

int main(void) {
    srand((unsigned int)time(NULL));

    static const char *scene_names[] = {
        "solar system", "binary star", "cluster",
    };
    static int (*const loaders[])(Planetoid *, Trail *, int, int) = {
        scene_solar_system,
        scene_binary_star,
        scene_random_cluster,
    };
    const int N_SCENES = 3;
    int scene_idx = 0;

    Planetoid *bodies = malloc(sizeof(Planetoid) * MAX_BODIES);
    Trail     *trails = malloc(sizeof(Trail)     * MAX_BODIES);
    if (!bodies || !trails) {
        fprintf(stderr, "allocation failed\n");
        free(bodies);
        free(trails);
        return 1;
    }

    int  n    = loaders[scene_idx](bodies, trails, MAX_BODIES, TRAIL_CAP);
    long step = 0;

    printf("initial energy: %.6e J\n", total_energy(bodies, n, G));

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(WIN_W, WIN_H, "nbody - N-body simulator");
    SetTargetFPS(60);
    ui_init();

    SimCamera cam = camera_default();

    /* Seed accelerations for the first half-kick of velocity Verlet. */
    apply_grav(bodies, n, G, EPS);

    while (!WindowShouldClose()) {
        handle_input(&cam);

        /* --- scene switching (1 / 2 / 3) --- */
        int new_scene = -1;
        if (IsKeyPressed(KEY_ONE))   new_scene = 0;
        if (IsKeyPressed(KEY_TWO))   new_scene = 1;
        if (IsKeyPressed(KEY_THREE)) new_scene = 2;

        if (new_scene >= 0 && new_scene < N_SCENES && new_scene != scene_idx) {
            unload_sim(bodies, trails, n);
            scene_idx = new_scene;
            n    = loaders[scene_idx](bodies, trails, MAX_BODIES, TRAIL_CAP);
            step = 0;
            cam  = camera_default();
            apply_grav(bodies, n, G, EPS);
        }

        /* --- add / remove bodies (A / D) --- */
        if (IsKeyPressed(KEY_A)) {
            int new_n = add_random_body(bodies, trails, n, MAX_BODIES, TRAIL_CAP, G);
            if (new_n > n) {
                n = new_n;
                apply_grav(bodies, n, G, EPS);
            }
        }
        if (IsKeyPressed(KEY_D) && n > 1) {
            body_free(&bodies[n - 1]);
            trail_free(&trails[n - 1]);
            n--;
            apply_grav(bodies, n, G, EPS);
        }

        /* --- physics --- */
        if (!cam.paused) {
            for (int s = 0; s < SUBSTEPS; s++) {
                integrate(bodies, n, DT);
                apply_grav(bodies, n, G, EPS);
                integrate_finish(bodies, n, DT);
                for (int i = 0; i < n; i++)
                    trail_push(&trails[i], bodies[i].pos);
                step++;
            }
        }
        camera_autofit(&cam, bodies, n);

        double E = total_energy(bodies, n, G);
        render_frame(trails, bodies, n, step, E, DT, GetFPS(), cam,
                     scene_names[scene_idx]);
    }

    ui_shutdown();
    CloseWindow();

    unload_sim(bodies, trails, n);
    free(bodies);
    free(trails);
    return 0;
}
