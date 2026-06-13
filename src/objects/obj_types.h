#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

typedef struct { 
    double x, y, z;
} Vec3;

typedef struct {
    Vec3 pos, vel, acc, acc_prev;
    long double mass, radius;
    char *name;
} Planetoid;

typedef struct {
    Vec3 *points;
    int capacity, head, count;
} Trail;

/* View/camera state for the renderer. Kept free of any raylib types so the
 * physics and object layers never need to depend on the graphics library. */
typedef struct {
    double scale;      // pixels per world-meter (zoom)
    double off_x;      // screen-space pan offset, in pixels
    double off_y;
    int    paused;     // 0 = running, 1 = paused
    int    auto_fit;   // 1 = camera auto-rescales to keep all bodies in frame
} SimCamera;

typedef struct OctNode {
    Vec3 center, half; // center point of the box, and half-width of the box
    Vec3 com; // center of mass of everything in the tree
    long double mass; // total mass of the tree 
    int body_idx; // -1 means non-existent
    struct OctNode *children[8];
} OctNode;



#endif