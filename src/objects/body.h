#ifndef BODY_h
#define BODY_h

#include "obj_types.h"

Planetoid body_init(Vec3 pos, Vec3 vel, long double mass, long double radius, char *name);
void body_free(Planetoid *p);

Trail trail_init(int capacity);
void trail_free(Trail *t);
void trail_push(Trail *t, Vec3 pos);

#endif
