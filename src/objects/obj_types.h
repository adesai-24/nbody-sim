#ifndef TYPES_H
#define TYPES_H

typedef struct { 
    double x, y, z;
} Vec3;

typedef struct {
    Vec3 pos, vel, acc, acc_prev;
    long double mass, radius;
    char *name;
} Planetoid;

typedef struct {
    Vec3 *points;
    int capacity, head, count;
} Trail;

#endif