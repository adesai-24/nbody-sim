#ifndef RENDER_H
#define RENDER_H

#include "objects/obj_types.h"

/* Build a camera centred on the window with a sensible default zoom
 * (one AU spans DEFAULT_AU_PX pixels). Call after InitWindow(). */
SimCamera camera_default(void);

/* Process one frame of input: scroll = zoom (about the cursor),
 * left-drag = pan, R = reset camera, Space = toggle pause. */
void handle_input(SimCamera *cam);

/* Draw fading motion trails behind each body. */
void draw_trails(Trail *trails, Planetoid *bodies, int n, SimCamera cam);

/* Draw each body as a colored circle, sized by the cube root of its mass. */
void draw_bodies(Planetoid *bodies, int n, SimCamera cam);

/* Overlay text: step count, body count, total energy, dt, FPS, controls. */
void draw_hud(Planetoid *bodies, int n, long step, double energy, double dt, int fps);

/* Bottom-right scale bar with a zoom-adaptive distance label (m/km/AU/ly). */
void draw_scale_legend(SimCamera cam);

#endif
