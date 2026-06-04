#include "body.h"

Planetoid body_init(Vec3 pos, Vec3 vel, long double mass, long double radius, char *name) {
    Planetoid new_planet;
    new_planet.pos = pos;
    new_planet.vel = vel;
    new_planet.mass = mass;
    new_planet.radius = radius;
    new_planet.name = name;
    new_planet.acc = (Vec3){0,0,0};
    new_planet.acc_prev = (Vec3){0,0,0};
    return new_planet;
}

Trail trail_init(int capacity) {
    Trail trail;
    trail.capacity = capacity;
    trail.head = 0;
    trail.count = 0;
    trail.points = malloc(sizeof(Vec3)*capacity);
    return trail; 
}

