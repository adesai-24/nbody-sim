#ifndef RENDER_H
#define RENDER_H

#include "objects/obj_types.h"

/* Build a camera centred on the window with a sensible default zoom
 * (one AU spans DEFAULT_AU_PX pixels). Call after InitWindow(). */
SimCamera camera_default(void);

/* Process one frame of input: scroll = zoom (about the cursor),
 * left-drag = pan, F = toggle auto-fit, R = reset camera, Space = toggle
 * pause. A manual zoom or pan turns auto-fit off. */
void handle_input(SimCamera *cam);

/* When auto_fit is on, smoothly rescale and re-center the camera so every
 * body stays in frame as the system expands or collapses. No-op otherwise. */
void camera_autofit(SimCamera *cam, const Planetoid *bodies, int n);

/* Draw fading motion trails behind each body. */
void draw_trails(Trail *trails, Planetoid *bodies, int n, SimCamera cam);

/* Draw each body as a colored circle, sized by the cube root of its mass. */
void draw_bodies(Planetoid *bodies, int n, SimCamera cam);

/* Overlay text: step count, body count, total energy, dt, FPS, controls. */
void draw_hud(Planetoid *bodies, int n, long step, double energy, double dt, int fps);

#endif
