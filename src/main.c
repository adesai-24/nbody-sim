#include <stdio.h>
#include <stdlib.h>
#include "objects/obj_types.h"
#include "body.h"
#include "physics.h"
#include <math.h>

#define G 6.674e-11
#define EPS 1e6
#define DT 3600.0
#define SUBSTEPS 24
#define MAX_STEPS 100000


void print_energy(Planetoid *bodies, int n, double grav_const) {
    double KE = 0.0;
    double PE = 0.0;
    for (int i = 0; i < n; i++) {
        Planetoid *a = &bodies[i];
        KE += 0.5*a->mass*(a->vel.x*a->vel.x + a->vel.y*a->vel.y + a->vel.z*a->vel.z);
        for (int j = i+1; j < n; j++) {
            Planetoid *b = &bodies[j];
            double dx = b->pos.x - a->pos.x;
            double dy = b->pos.y - a->pos.y;
            double dz = b->pos.z - a->pos.z;

            double r = sqrt(dx*dx + dy*dy + dz*dz);
            PE += -(grav_const) * b->mass * a->mass / r;
        }
    }
    double total = KE + PE;
    printf("Total Energy: %e\n", total);
    printf("Kinetic Energy: %e\n", KE);
    printf("Potential Energy: %e\n", PE);
}

int main() {
    int n = 2;
    Planetoid *bodies = malloc(sizeof(Planetoid)*n);
    Trail *trails = malloc(sizeof(Trail)*n);

    bodies[0] = body_init((Vec3){0,0,0}, (Vec3){0,0,0}, 1.989e30, 69634e3, "Sun"); // hard-coded sun
    bodies[1] = body_init((Vec3){1.496e11, 0, 0}, (Vec3){0,29783,0}, 5.972e24, 6371e3, "Earth"); // hard-coded earth

    trails[0] = trail_init(300);
    trails[1] = trail_init(300);

    for (int step = 0; step < MAX_STEPS; step++) {
        apply_grav(bodies, n, G, EPS);
        integrate(bodies, n, DT);
        if (step % 100 == 0) {
            print_energy(bodies, n, G);
        }
    }

    for (int i = 0; i < n; i++) {
        body_free(&bodies[i]);
        trail_free(&trails[i]);
    }
    free(bodies);
    free(trails);

    return 0;
}
