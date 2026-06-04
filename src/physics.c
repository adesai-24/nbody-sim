#include "physics.h"

void compute_forces(Planetoid *bodies, int n, double G, double eps) {
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

void integrate(Planetoid *bodies, int n, double dt) {
    for (int i = 0; i < n; i++) {
        Planetoid *a = &bodies[i];
        Vec3 term1_pos = vec_scale_res(a->vel, dt);
        Vec3 term2_pos = vec_scale_res(a->acc_prev, dt*dt*0.5);
        Vec3 pos_res = vec_add_res(term1_pos, term2_pos);
        vec_add(&(a->pos), pos_res);

        Vec3 term1_vel = vec_add_res(a->acc_prev, a->acc);
        vec_scale(&term1_vel, dt*0.5);
        vec_add(&(a->vel), term1_vel);
        a->acc_prev = a->acc;
        a->acc = (Vec3){0,0,0};
    }
}