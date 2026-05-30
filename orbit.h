#ifndef ORBIT_H
#define ORBIT_H

#include "planet.h"

typedef struct {
    int num_of_planetoids;
    char *name;
    Planetoid *planets;
    Point3D center;
    Planetoid *host;
} Orbit;

void *simulate;


#endif