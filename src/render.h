#ifndef RENDER_H
#define RENDER_H

#include "objects/obj_types.h"

/* Load UI fonts, the glass shader, and offscreen render targets (call once
 * after InitWindow). ui_shutdown() frees them (call before CloseWindow).
 * Fonts fall back to the built-in font if assets/fonts/ is missing. */
void ui_init(void);
void ui_shutdown(void);

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

/* Render one full frame: the gradient backdrop, motion trails and bodies, a
 * blurred copy of that scene, and the frosted-glass HUD composited on top.
 * Call between (and instead of) BeginDrawing()/EndDrawing() in the loop. */
void render_frame(Trail *trails, Planetoid *bodies, int n,
                  long step, double energy, double dt, int fps, SimCamera cam);

#endif
