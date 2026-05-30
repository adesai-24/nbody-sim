#ifndef PLANET_H
#define PLANET_H

typedef struct Orbit Orbit;

typedef struct {
    double x;
    double y;
    double z;
} Point3D;

typedef struct {
    long double mass;
    char *classification;
    long double radius;
    double rotation_rate;
    char *proximity_ordering;
    double orbital_period;
    char *name;
    Point3D point;
    Orbit *orbit;
} Planetoid;

#endif 




