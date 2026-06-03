#include "obj_types.h"
#include <math.h>

void *vec_add(Vec3 *dest, Vec3 *src) {
    dest->x += src->x;
    dest->y += src->y;
    dest->z += src->z;
}

void vec_scale(Vec3 *vec, double scaler) {
    if (scaler <= 0.0) {
        printf("Please provide a value greater than 0!\n");
        return;
    }
    vec->x *= scaler;
    vec->y *= scaler;
    vec->z *= scaler;
}
double vec_magnitude(Vec3 vec) {
    return sqrt(pow(vec.x, 2) + pow(vec.y, 2) + pow(vec.z, 2));
}