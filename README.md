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

| Input            | Action            |
| ---------------- | ----------------- |
| Scroll wheel     | Zoom in / out     |
| Left-click drag  | Pan the view      |
| `R`              | Reset the camera  |
| `Space`          | Pause / resume    |
| `Esc`            | Quit              |

The HUD overlays the step count, body count, total system energy, timestep, and
FPS. Total energy should stay flat — that is the correctness check that the
physics is conserving energy.

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
