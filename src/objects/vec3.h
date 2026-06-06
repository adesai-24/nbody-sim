#ifndef VEC3_H
#define VEC3_h

#include "obj_types.h"

void vec_add(Vec3 *vec1, Vec3 vec2);
Vec3 vec_add_res(Vec3 v1, Vec3 v2);
void vec_scale(Vec3 *vec, double scaler);
Vec3 vec_scale_res(Vec3 vec, double scaler);
double vec_magnitude(Vec3 vec);

#endif