#include "obj_types.h"
#include <math.h>

Vec3 vec_add(double x, double y, double z) {
    Vec3 vec;
    vec.x = x;
    vec.y = y;
    vec.z = z;
    return vec;
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