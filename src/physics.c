#include "physics.h"

void apply_grav(Planetoid *bodies, int n, double G, double eps) {
    for (int i = 0; i < n; i++) {
        bodies[i].acc = (Vec3){0,0,0};
    }
    for (int i = 0; i < n; i++) {
        for (int j = i+1; j < n; j++) {
            Planetoid *a = &bodies[i];
            Planetoid *b = &bodies[j];
            double dx = b->pos.x - a->pos.x;
            double dy = b->pos.y - a->pos.y;
            double dz = b->pos.z - a->pos.z;

            double r = sqrt(dx*dx + dy*dy + dz*dz + eps*eps);
            double force = (G*a->mass*b->mass)/(r*r);
            
            a->acc.x += (force / a->mass) * (dx/r);
            a->acc.y += (force / a->mass) * (dy/r);
            a->acc.z += (force / a->mass) * (dz/r);

            b->acc.x -= (force / b->mass) * (dx/r);
            b->acc.y -= (force / b->mass) * (dy/r);
            b->acc.z -= (force / b->mass) * (dz/r);
        }
    }
}

/* Velocity Verlet, first half-step: kick the velocity by half a step using the
 * acceleration at the *current* position, then drift the position forward.
 *
 *   v_half = v + 0.5 * a(x) * dt
 *   x_new  = x + v_half * dt
 *
 * Run as:  apply_grav();        // seed a(x_0) once before the loop
 *          integrate();         // half-kick + drift
 *          apply_grav();        // a(x_new)
 *          integrate_finish();  // closing half-kick
 *
 * The earlier code drove the position update from acc_prev (the *previous*
 * step's acceleration), which is not velocity Verlet and bled ~1% of the
 * system energy per 10k steps -- enough to visibly spiral the orbit inward
 * over an interactive session. Splitting the kick around the force evaluation
 * makes the integrator symplectic, so energy stays flat indefinitely. */
void integrate(Planetoid *bodies, int n, double dt) {
    for (int i = 0; i < n; i++) {
        Planetoid *a = &bodies[i];
        a->vel.x += 0.5 * a->acc.x * dt;
        a->vel.y += 0.5 * a->acc.y * dt;
        a->vel.z += 0.5 * a->acc.z * dt;

        a->pos.x += a->vel.x * dt;
        a->pos.y += a->vel.y * dt;
        a->pos.z += a->vel.z * dt;
    }
}

/* Velocity Verlet, closing half-step: kick the velocity by the remaining half
 * step using the freshly recomputed acceleration at the new position.
 *
 *   v_new = v_half + 0.5 * a(x_new) * dt   */
void integrate_finish(Planetoid *bodies, int n, double dt) {
    for (int i = 0; i < n; i++) {
        Planetoid *a = &bodies[i];
        a->vel.x += 0.5 * a->acc.x * dt;
        a->vel.y += 0.5 * a->acc.y * dt;
        a->vel.z += 0.5 * a->acc.z * dt;
    }
}

/* KE = sum(0.5 * m * |v|^2),  PE = sum over pairs(-G * m_i * m_j / r).
 * Returned total should stay within ~1% of its initial value (Phase 1 gate)
 * and is what the HUD reports so drift is visible at a glance. */
double total_energy(Planetoid *bodies, int n, double G) {
    double KE = 0.0;
    double PE = 0.0;
    for (int i = 0; i < n; i++) {
        Planetoid *a = &bodies[i];
        double v2 = a->vel.x*a->vel.x + a->vel.y*a->vel.y + a->vel.z*a->vel.z;
        KE += 0.5 * (double)a->mass * v2;
        for (int j = i + 1; j < n; j++) {
            Planetoid *b = &bodies[j];
            double dx = b->pos.x - a->pos.x;
            double dy = b->pos.y - a->pos.y;
            double dz = b->pos.z - a->pos.z;
            double r = sqrt(dx*dx + dy*dy + dz*dz);
            PE += -G * (double)a->mass * (double)b->mass / r;
        }
    }
    return KE + PE;
}