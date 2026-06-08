#ifndef BARNES_HUT_H
#define BARNES_HUT_H

#include "../objects/obj_types.h"

void tree_insert(OctNode *node, Planetoid *bodies, int body_idx);
OctNode *create_child(OctNode *parent, int oct);
int find_octant(Planetoid body, Vec3 node);
OctNode *alloc_node();
void tree_mass(OctNode *node);

#endif