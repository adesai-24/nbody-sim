#ifndef PHYSICS_h
#define PHYSICS_h

#include "objects/body.h"
#include "objects/obj_types.h"
#include <math.h>

void apply_grav(Planetoid *bodies, int n, double G, double eps);
void integrate(Planetoid *bodies, int n, double dt);
void integrate_finish(Planetoid *bodies, int n, double dt);
double total_energy(Planetoid *bodies, int n, double G);

#endif
