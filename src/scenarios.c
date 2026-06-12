#define _POSIX_C_SOURCE 200809L
#include "scenarios.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define G_CONST  6.674e-11
#define PI       3.14159265358979323846

int scene_solar_system(Planetoid *b, Trail *t, int max, int trail_cap) {
    if (max < 9) return 0;
    int n = 0;

    /* All bodies start on the +x axis with prograde (+y) circular-orbit
     * velocity.  Masses, radii, and semi-major axes are mean values (SI). */
    b[n] = body_init((Vec3){0,          0, 0}, (Vec3){0,     0, 0}, 1.989e30, 696340e3, "Sun");
    t[n] = trail_init(trail_cap); n++;

    b[n] = body_init((Vec3){5.791e10,   0, 0}, (Vec3){0, 47870, 0}, 3.301e23,   2440e3, "Mercury");
    t[n] = trail_init(trail_cap); n++;

    b[n] = body_init((Vec3){1.082e11,   0, 0}, (Vec3){0, 35020, 0}, 4.867e24,   6052e3, "Venus");
    t[n] = trail_init(trail_cap); n++;

    b[n] = body_init((Vec3){1.496e11,   0, 0}, (Vec3){0, 29783, 0}, 5.972e24,   6371e3, "Earth");
    t[n] = trail_init(trail_cap); n++;

    b[n] = body_init((Vec3){2.279e11,   0, 0}, (Vec3){0, 24077, 0}, 6.390e23,   3390e3, "Mars");
    t[n] = trail_init(trail_cap); n++;

    b[n] = body_init((Vec3){7.783e11,   0, 0}, (Vec3){0, 13070, 0}, 1.898e27,  71492e3, "Jupiter");
    t[n] = trail_init(trail_cap); n++;

    b[n] = body_init((Vec3){1.432e12,   0, 0}, (Vec3){0,  9680, 0}, 5.683e26,  60268e3, "Saturn");
    t[n] = trail_init(trail_cap); n++;

    b[n] = body_init((Vec3){2.867e12,   0, 0}, (Vec3){0,  6800, 0}, 8.681e25,  25559e3, "Uranus");
    t[n] = trail_init(trail_cap); n++;

    b[n] = body_init((Vec3){4.515e12,   0, 0}, (Vec3){0,  5430, 0}, 1.024e26,  24764e3, "Neptune");
    t[n] = trail_init(trail_cap); n++;

    return n;
}

int scene_binary_star(Planetoid *b, Trail *t, int max, int trail_cap) {
    if (max < 2) return 0;
    int n = 0;

    /* Two stars in a mutual circular orbit, CoM at origin.
     *   m1 = 2.0e30 kg  (Alpha), m2 = 1.5e30 kg (Beta)
     *   separation d = 8.0e10 m (~0.53 AU), period ~107 days */
    double m1 = 2.0e30, m2 = 1.5e30;
    double M  = m1 + m2;
    double d  = 8.0e10;
    double r1 = d * m2 / M;              /* Alpha distance from CoM */
    double r2 = d * m1 / M;              /* Beta  distance from CoM */
    double omega = sqrt(G_CONST * M / (d * d * d));
    double v1 = r1 * omega;
    double v2 = r2 * omega;

    /* Alpha at -x, Beta at +x, both orbiting CCW (positive z angular momentum).
     * At (-r, 0) CCW velocity points in -y; at (+r, 0) it points in +y. */
    b[n] = body_init((Vec3){-r1, 0, 0}, (Vec3){0, -v1, 0}, m1, 800000e3, "Alpha");
    t[n] = trail_init(trail_cap); n++;

    b[n] = body_init((Vec3){ r2, 0, 0}, (Vec3){0,  v2, 0}, m2, 650000e3, "Beta");
    t[n] = trail_init(trail_cap); n++;

    /* Two circumbinary planets on stable circular orbits well outside the
     * binary (> 3x separation).  They orbit the system's total mass. */
    struct { double r; double mass; double rad; const char *name; } planets[] = {
        { 2.5e11, 5.972e24, 6371e3, "Kepler-b" },
        { 4.5e11, 3.285e23, 2440e3, "Kepler-c" },
    };
    int np = (int)(sizeof(planets) / sizeof(planets[0]));

    for (int i = 0; i < np && n < max; i++) {
        double r_p = planets[i].r;
        double v_p = sqrt(G_CONST * M / r_p);
        b[n] = body_init((Vec3){r_p, 0, 0}, (Vec3){0, v_p, 0},
                         planets[i].mass, planets[i].rad, planets[i].name);
        t[n] = trail_init(trail_cap); n++;
    }

    return n;
}

int scene_random_cluster(Planetoid *b, Trail *t, int max, int trail_cap) {
    int total = (max < 20) ? max : 20;

    srand(42);   /* fixed seed: cluster looks the same every time you press 3 */

    /* Compact central mass (think intermediate-mass black hole or nuclear
     * star cluster).  Orbiting bodies are stellar-mass objects. */
    double central_mass = 1.0e33;
    b[0] = body_init((Vec3){0, 0, 0}, (Vec3){0, 0, 0},
                     central_mass, 5000000e3, "Core");
    t[0] = trail_init(trail_cap);

    int n = 1;
    for (int i = 1; i < total; i++) {
        double r     = 1e12 + ((double)rand() / RAND_MAX) * 1.9e13;
        double angle = ((double)rand() / RAND_MAX) * 2.0 * PI;
        double mass  = 1e29 + ((double)rand() / RAND_MAX) * 4.9e30;
        double v     = sqrt(G_CONST * central_mass / r);

        double px =  r * cos(angle);
        double py =  r * sin(angle);
        double vx = -v * sin(angle);
        double vy =  v * cos(angle);

        char name[12];
        snprintf(name, sizeof(name), "S%d", i);
        b[n] = body_init((Vec3){px, py, 0}, (Vec3){vx, vy, 0},
                         mass, 500000e3, name);
        t[n] = trail_init(trail_cap);
        n++;
    }

    return n;
}

int add_random_body(Planetoid *b, Trail *t, int n, int max, int trail_cap,
                    double G) {
    if (n >= max) return n;

    /* Compute system center of mass and total mass. */
    double total_mass = 0.0, cx = 0.0, cy = 0.0;
    for (int i = 0; i < n; i++) {
        double m = (double)b[i].mass;
        total_mass += m;
        cx += m * b[i].pos.x;
        cy += m * b[i].pos.y;
    }
    if (total_mass > 0.0) { cx /= total_mass; cy /= total_mass; }

    /* Random angle and distance 1–5 AU from CoM. */
    double angle = ((double)rand() / RAND_MAX) * 2.0 * PI;
    double r     = 1.496e11 * (1.0 + ((double)rand() / RAND_MAX) * 4.0);
    double px    = cx + r * cos(angle);
    double py    = cy + r * sin(angle);

    /* Near-circular orbit velocity around total system mass. */
    double v  = sqrt(G * total_mass / r);
    double vx = -v * sin(angle);
    double vy =  v * cos(angle);

    /* Small random mass: sub-Earth to roughly Earth-sized. */
    double mass = 1e22 + ((double)rand() / RAND_MAX) * 5e24;

    char name[16];
    snprintf(name, sizeof(name), "X%d", n);
    b[n] = body_init((Vec3){px, py, 0}, (Vec3){vx, vy, 0}, mass, 3000e3, name);
    t[n] = trail_init(trail_cap);

    return n + 1;
}
