#ifndef SCENARIOS_H
#define SCENARIOS_H

#include "objects/body.h"

#define MAX_BODIES 128

/* Load a preset into b/t[0..max-1]; caller must free any prior bodies first.
 * Returns the number of bodies loaded. */
int scene_solar_system(Planetoid *b, Trail *t, int max, int trail_cap);
int scene_binary_star(Planetoid *b, Trail *t, int max, int trail_cap);
int scene_random_cluster(Planetoid *b, Trail *t, int max, int trail_cap);

/* Append one body on a roughly circular orbit around the system's CoM.
 * Returns the new body count (unchanged if already at max). */
int add_random_body(Planetoid *b, Trail *t, int n, int max, int trail_cap,
                    double G);

#endif
