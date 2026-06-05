# NBody

A real-time 3D gravitational N-body simulator in C. Bodies attract each other
under Newtonian gravity; their motion is integrated with velocity Verlet and
rendered live with [raylib](https://www.raylib.com/).

## Building

Install raylib, then run `make`:

```sh
# Ubuntu / Debian
sudo apt install libraylib-dev
# macOS
brew install raylib
# MSYS2
pacman -S mingw-w64-x86_64-raylib

make
./nbody
```

The Makefile uses `pkg-config` to locate raylib when available and otherwise
links `-lraylib` directly. Build flags: `-Wall -Wextra -O2 -std=c11`.

## Controls

| Input            | Action                       |
| ---------------- | ---------------------------- |
| Scroll wheel     | Zoom in / out (manual)       |
| Left-click drag  | Pan the view                 |
| `F`              | Toggle auto fit-to-view zoom |
| `R`              | Reset the camera             |
| `Space`          | Pause / resume               |
| `Esc`            | Quit                         |

By default the camera **auto-fits**: it continuously rescales and re-centers so
every body stays in frame as the system expands or collapses. Scrolling or
dragging hands control back to you (turning auto-fit off); press `F` to toggle
it again, or `R` to reset the view and re-enable it.

Bodies are drawn at a size that tracks both their mass and the current zoom, so
they shrink as you zoom out and grow as you zoom in. A map-style **scale bar** in
the bottom-right shows the on-screen distance for a round number of AU (or km
when zoomed in close).

The interface uses a **glassmorphism** style over a dark navy gradient: the scene
is rendered to an offscreen buffer, blurred, and the HUD is composited on top as
a frosted-glass card (a small GLSL shader masks the blurred backdrop to a rounded
rectangle and adds the tint and edge highlight). Bodies are drawn with a soft
glow. Text uses the bundled [Outfit](https://github.com/Outfitio/Outfit-fonts)
sans-serif. The card shows the step count, body count, total system energy,
timestep, FPS, and the current view mode (`AUTO`/`MANUAL`). Total energy should
stay flat — that is the correctness check that the physics is conserving energy.

The renderer needs OpenGL 3.3; if the glass shader can't load it falls back to a
plain translucent panel, and if `assets/fonts/` is missing it falls back to the
built-in font. Run the binary from the repository root so it can find
`assets/fonts/`.

## Layout

```
src/
  main.c                 entry point: owns the bodies, runs the window loop
  physics.h / physics.c  apply_grav(), velocity-Verlet integrate(), total_energy()
  render.h  / render.c   camera, draw_bodies(), draw_trails(), draw_hud()
  objects/
    obj_types.h          Vec3, Planetoid, Trail, SimCamera
    obj_types.c          vector helpers
    body.h / body.c      body_init/free, trail_init/push/free
```

## Physics notes

- Forces are summed pairwise (O(n²)) with a softening term `eps` to avoid
  singularities at very small separations.
- Integration is **velocity Verlet**, split around the force evaluation
  (half-kick → drift → recompute forces → half-kick). This symplectic scheme
  keeps total energy flat over long runs rather than letting orbits spiral.
- Units are SI (kilograms, metres, seconds) so `G = 6.674e-11` applies directly.
