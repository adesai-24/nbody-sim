#ifndef PHYSICS_h
#define PHYSICS_h

#include "objects/body.h"
#include "objects/obj_types.h"
#include <math.h>

void compute_forces(Planetoid *bodies, int n, double G, double eps);
void integrate(Planetoid *bodies, int n, double dt);

#endif