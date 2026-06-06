#include "barnes_hut.h"
#include "../objects/vec3.h"
#include "../objects/sim_state.h"
#include <math.h>
#include <stdio.h>

/*
Here is a visualization of the box
oct 0  →  x-, y-, z- (left,  below, back)
oct 1  →  x+, y-, z- (right, below, back)
oct 2  →  x-, y+, z- (left,  above, back)
oct 3  →  x+, y+, z- (right, above, back)
oct 4  →  x-, y-, z+ (left,  below, front)
oct 5  →  x+, y-, z+ (right, below, front)
oct 6  →  x-, y+, z+ (left,  above, front)
oct 7  →  x+, y+, z+ (right, above, front)
*/
void tree_insert(OctNode *node, Planetoid *bodies, int body_idx) {
    if (node->body_idx == -1 && node->children[0] == NULL && node->children[7] == NULL) {
        node->body_idx = body_idx;
        node->mass = bodies[body_idx].mass;
        node->com = bodies[body_idx].pos;
    }
    else if (node->body_idx != -1) {
        int exist_idx = node->body_idx;
        node->body_idx = -1;
        int exist_oct = find_octant(bodies[exist_idx], node->center);
        if (node->children[exist_oct] == NULL) {
            node->children[exist_oct] = create_child(node, exist_oct);
        }
        tree_insert(node->children[exist_oct], bodies, exist_idx);
        tree_insert(node, bodies, body_idx);
        
    }
    else {
        int oct = find_octant(bodies[body_idx], node->center);
        if (node->children[oct] == NULL) {
            node->children[oct] = create_child(node, oct);
        }
        tree_insert(node->children[oct], bodies, body_idx);
    }

}

// this is for creating a child with the proper subsections
OctNode *create_child(OctNode *parent, int oct) {
    OctNode *child = alloc_node();
    child->half = vec_scale_res(parent->half, 0.5);
    child->center = parent->center;
    if (oct & 1) {
        child->center.x += child->half.x;
    } else {
        child->center.x -= child->half.x;
    }
    if (oct & 2) {
        child->center.y += child->half.y;
    } else {
        child->center.y -= child->half.y;
    }
    if (oct & 4) {
        child->center.z += child->half.z;
    } else {
        child->center.z -= child->half.z;
    }
    return child;
}

int find_octant(Planetoid body, Vec3 node) {
    int oct = 0;
    if (body.pos.x > node.x) {
        oct |= 1;
    }
    if (body.pos.y > node.y) {
        oct |= 2;
    }
    if (body.pos.z > node.z) {
        oct |= 4;
    }
    return oct;
}

OctNode *alloc_node() {
    OctNode *node = &pool[pool_idx++];
    node->body_idx = -1;
    node->mass = 0;
    node->com = (Vec3){0,0,0};
    for (int i = 0; i < 8; i++) node->children[i] = NULL;
    return node;
}
