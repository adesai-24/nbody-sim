#include "vec3.h"
#include <math.h>

void vec_add(Vec3 *dest, Vec3 src) {
    dest->x += src.x;
    dest->y += src.y;
    dest->z += src.z;
}

Vec3 vec_add_res(Vec3 v1, Vec3 v2) {
    Vec3 res;
    res.x = v1.x + v2.x;
    res.y = v1.y + v2.y;
    res.z = v1.z + v2.z;
    return res;
}

void vec_scale(Vec3 *vec, double scaler) {
    vec->x *= scaler;
    vec->y *= scaler;
    vec->z *= scaler;
}

Vec3 vec_scale_res(Vec3 vec, double scaler) {
    vec.x *= scaler;
    vec.y *= scaler;
    vec.z *= scaler;
    return vec;
}

double vec_magnitude(Vec3 vec) {
    return sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}
