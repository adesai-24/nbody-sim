#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <raylib.h>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

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

static const char *SCENE_NAMES[] = {
    "solar system", "binary star", "cluster",
};
static int (*const LOADERS[])(Planetoid *, Trail *, int, int) = {
    scene_solar_system,
    scene_binary_star,
    scene_random_cluster,
};
static const int N_SCENES = 3;

/* All simulation state as a single static struct so that the per-frame
 * callback used by Emscripten's main loop can access it. */
static struct {
    Planetoid *bodies;
    Trail     *trails;
    int        n;
    long       step;
    SimCamera  cam;
    int        scene_idx;
} S;

static void unload_sim(void) {
    for (int i = 0; i < S.n; i++) {
        body_free(&S.bodies[i]);
        trail_free(&S.trails[i]);
    }
    S.n = 0;
}

static void load_scene(int idx) {
    unload_sim();
    S.scene_idx = idx;
    S.n    = LOADERS[idx](S.bodies, S.trails, MAX_BODIES, TRAIL_CAP);
    S.step = 0;
    S.cam  = camera_default();
    apply_grav(S.bodies, S.n, G, EPS);
}

static void update_draw_frame(void) {
    handle_input(&S.cam);

    /* Scene switching: 1 / 2 / 3 */
    int new_scene = -1;
    if (IsKeyPressed(KEY_ONE))   new_scene = 0;
    if (IsKeyPressed(KEY_TWO))   new_scene = 1;
    if (IsKeyPressed(KEY_THREE)) new_scene = 2;
    if (new_scene >= 0 && new_scene < N_SCENES && new_scene != S.scene_idx)
        load_scene(new_scene);

    /* Add / remove bodies: A / D */
    if (IsKeyPressed(KEY_A)) {
        int new_n = add_random_body(S.bodies, S.trails, S.n, MAX_BODIES,
                                    TRAIL_CAP, G);
        if (new_n > S.n) {
            S.n = new_n;
            apply_grav(S.bodies, S.n, G, EPS);
        }
    }
    if (IsKeyPressed(KEY_D) && S.n > 1) {
        body_free(&S.bodies[S.n - 1]);
        trail_free(&S.trails[S.n - 1]);
        S.n--;
        apply_grav(S.bodies, S.n, G, EPS);
    }

    /* Physics */
    if (!S.cam.paused) {
        for (int s = 0; s < SUBSTEPS; s++) {
            integrate(S.bodies, S.n, DT);
            apply_grav(S.bodies, S.n, G, EPS);
            integrate_finish(S.bodies, S.n, DT);
            for (int i = 0; i < S.n; i++)
                trail_push(&S.trails[i], S.bodies[i].pos);
            S.step++;
        }
    }
    camera_autofit(&S.cam, S.bodies, S.n);

    double E = total_energy(S.bodies, S.n, G);
    render_frame(S.trails, S.bodies, S.n, S.step, E, DT, GetFPS(), S.cam,
                 SCENE_NAMES[S.scene_idx]);
}

int main(void) {
    srand((unsigned int)time(NULL));

    S.bodies = malloc(sizeof(Planetoid) * MAX_BODIES);
    S.trails = malloc(sizeof(Trail)     * MAX_BODIES);
    if (!S.bodies || !S.trails) {
        fprintf(stderr, "allocation failed\n");
        free(S.bodies);
        free(S.trails);
        return 1;
    }

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(WIN_W, WIN_H, "nbody - N-body simulator");
    SetTargetFPS(60);
    ui_init();

    load_scene(0);
    printf("initial energy: %.6e J\n", total_energy(S.bodies, S.n, G));

#if defined(PLATFORM_WEB)
    /* Emscripten requires a callback-based loop; 60 = target FPS,
     * 1 = simulate infinite loop (main() does not return). */
    emscripten_set_main_loop(update_draw_frame, 60, 1);
#else
    while (!WindowShouldClose()) update_draw_frame();
#endif

    ui_shutdown();
    CloseWindow();

    unload_sim();
    free(S.bodies);
    free(S.trails);
    return 0;
}
