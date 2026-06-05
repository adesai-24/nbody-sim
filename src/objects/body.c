#define _POSIX_C_SOURCE 200809L  /* expose strdup() under -std=c11 */
#include "body.h"
#include <stdlib.h>
#include <string.h>

Planetoid body_init(Vec3 pos, Vec3 vel, long double mass, long double radius, char *name) {
    Planetoid new_planet;
    new_planet.pos = pos;
    new_planet.vel = vel;
    new_planet.mass = mass;
    new_planet.radius = radius;
    new_planet.name = name ? strdup(name) : NULL;
    new_planet.acc = (Vec3){0,0,0};
    new_planet.acc_prev = (Vec3){0,0,0};
    return new_planet;
}

void body_free(Planetoid *p) {
    if (!p) return;
    free(p->name);
    p->name = NULL;
}

Trail trail_init(int capacity) {
    Trail trail;
    trail.capacity = capacity;
    trail.head = 0;
    trail.count = 0;
    trail.points = malloc(sizeof(Vec3) * capacity);
    return trail;
}

void trail_free(Trail *t) {
    if (!t) return;
    free(t->points);
    t->points = NULL;
    t->capacity = 0;
    t->head = 0;
    t->count = 0;
}

void trail_push(Trail *t, Vec3 pos) {
    t->head = (t->head + 1) % t->capacity;
    t->points[t->head] = pos;
    if (t->count < t->capacity) t->count++;
}
