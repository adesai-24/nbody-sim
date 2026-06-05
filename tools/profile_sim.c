/* Standalone profiling harness for the N-body physics core.
 *
 * Links only the raylib-free translation units (physics.c, body.c,
 * obj_types.c) so it can run headless. For each N it builds a random cloud of
 * bodies, runs the exact velocity-Verlet substep sequence the live sim uses
 * (integrate -> apply_grav -> integrate_finish), plus the per-frame
 * total_energy() call, and times each phase with clock_gettime(CLOCK_MONOTONIC).
 *
 * The goal is to confirm whether apply_grav (the O(n^2) pairwise force sum) is
 * the bottleneck, and how its share of the step grows with N.
 *
 * Build:  see tools/Makefile  ->  make -C tools profile_sim
 * Run:    ./tools/profile_sim [steps] [N ...]
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "objects/obj_types.h"
#include "physics.h"

/* Match the constants the live sim runs with (see src/main.c). */
#define G    6.674e-11
#define EPS  1e6
#define DT   3600.0

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

/* Deterministic pseudo-random in [0,1) so runs are reproducible. */
static double frand(unsigned int *seed) {
    *seed = *seed * 1103515245u + 12345u;
    return (double)((*seed >> 8) & 0xFFFFFF) / (double)0x1000000;
}

/* Build a roughly solar-scale random cloud: positions within ~10 AU, modest
 * velocities, planet-to-star masses. Values only need to exercise the same
 * arithmetic the real sim does -- the timing does not depend on the physics
 * being a stable system. */
static void seed_bodies(Planetoid *b, int n, unsigned int seed) {
    const double AU = 1.496e11;
    for (int i = 0; i < n; i++) {
        b[i].pos = (Vec3){ (frand(&seed) - 0.5) * 20 * AU,
                           (frand(&seed) - 0.5) * 20 * AU,
                           (frand(&seed) - 0.5) * 20 * AU };
        b[i].vel = (Vec3){ (frand(&seed) - 0.5) * 6e4,
                           (frand(&seed) - 0.5) * 6e4,
                           (frand(&seed) - 0.5) * 6e4 };
        b[i].acc = b[i].acc_prev = (Vec3){0,0,0};
        b[i].mass   = 1e24 + frand(&seed) * 2e30;
        b[i].radius = 6e6;
        b[i].name   = NULL;
    }
}

static void profile_n(int n, int steps) {
    Planetoid *bodies = malloc(sizeof(Planetoid) * (size_t)n);
    if (!bodies) { fprintf(stderr, "alloc failed for n=%d\n", n); exit(1); }
    seed_bodies(bodies, n, 0xC0FFEEu + (unsigned)n);

    double t_grav = 0, t_integ = 0, t_energy = 0;
    double t0;

    /* Seed accelerations once, exactly as the sim does before its loop. */
    apply_grav(bodies, n, G, EPS);

    for (int s = 0; s < steps; s++) {
        t0 = now_sec();
        integrate(bodies, n, DT);
        t_integ += now_sec() - t0;

        t0 = now_sec();
        apply_grav(bodies, n, G, EPS);
        t_grav += now_sec() - t0;

        t0 = now_sec();
        integrate_finish(bodies, n, DT);
        t_integ += now_sec() - t0;

        /* The live sim calls total_energy() once per rendered frame for the
         * HUD; it is also O(n^2), so it is worth measuring alongside. */
        t0 = now_sec();
        volatile double E = total_energy(bodies, n, G);
        (void)E;
        t_energy += now_sec() - t0;
    }

    double t_total = t_grav + t_integ + t_energy;
    double us_per_step_grav = t_grav / steps * 1e6;

    printf("  n=%-5d steps=%-7d | apply_grav %8.2f ms (%5.1f%%)  "
           "integrate %7.2f ms (%5.1f%%)  total_energy %8.2f ms (%5.1f%%)\n",
           n, steps,
           t_grav   * 1e3, 100.0 * t_grav   / t_total,
           t_integ  * 1e3, 100.0 * t_integ  / t_total,
           t_energy * 1e3, 100.0 * t_energy / t_total);
    printf("        -> apply_grav = %.3f us/step, %.4f us/pair (%d pairs)\n",
           us_per_step_grav,
           us_per_step_grav / (n * (n - 1) / 2.0),
           n * (n - 1) / 2);

    free(bodies);
}

int main(int argc, char **argv) {
    int steps = (argc > 1) ? atoi(argv[1]) : 2000;

    int default_ns[] = {100, 200, 500, 1000};
    int *ns;
    int count;
    if (argc > 2) {
        count = argc - 2;
        ns = malloc(sizeof(int) * (size_t)count);
        for (int i = 0; i < count; i++) ns[i] = atoi(argv[i + 2]);
    } else {
        count = (int)(sizeof(default_ns) / sizeof(default_ns[0]));
        ns = default_ns;
    }

    printf("N-body physics profile (CLOCK_MONOTONIC), %d substeps per N\n", steps);
    printf("G=%.3e EPS=%.0e DT=%.0f\n\n", (double)G, (double)EPS, (double)DT);

    for (int i = 0; i < count; i++)
        profile_n(ns[i], steps);

    if (argc > 2) free(ns);
    return 0;
}
