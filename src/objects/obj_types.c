#include "obj_types.h"
#include <math.h>
#include <stdio.h>

// global variable definitions
Planetoid *bodies;
Trail *trails;
OctNode *pool;
int pool_idx;

// Vec helper functions
void vec_add(Vec3 *dest, Vec3 src) {
    dest->x += src.x;
    dest->y += src.y;
    dest->z += src.z;
}

Vec3 vec_add_res(Vec3 v1, Vec3 v2) {
    Vec3 res;
    res.x = v1.x + v2.x;
    res.y = v1.y + v2.y;
    res.z = v1.z + v2.z;
    return res;
}

void vec_scale(Vec3 *vec, double scaler) {
    vec->x *= scaler;
    vec->y *= scaler;
    vec->z *= scaler;
}

Vec3 vec_scale_res(Vec3 vec, double scaler) {
    vec.x *= scaler;
    vec.y *= scaler;
    vec.z *= scaler;
    return vec;
}

double vec_magnitude(Vec3 vec) {
    return sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}


// OctNode helper functions
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
    return &pool[pool_idx++];
}

void free_node(OctNode *node) {
    if (!node) return;
    for (int i = 0; i < 8; i++) free_node(node->children[i]);
    free(node->children);
    free(node);
}