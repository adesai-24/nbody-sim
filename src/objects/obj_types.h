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

void *vec_add(Vec3 *vec1, Vec3 vec2);
Vec3 vec_add_res(Vec3 v1, Vec3 v2);
void *vec_scale(Vec3 *vec, double scaler);
Vec3 vec_scale_res(Vec3 vec, double scaler);
double vec_magnitude(Vec3 vec);

#endif