#ifndef BODY_h
#define BODY_h

#include "obj_types.h"

Planetoid body_init(Vec3 pos, Vec3 vel, long double mass, long double radius, char *name);
Trail trail_init(int capacity);

#endif