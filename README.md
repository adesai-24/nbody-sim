# NBody

**[▶ Try it live in your browser](https://adesai-24.github.io/nbody-sim/)**

A real-time gravitational N-body simulator in C. Bodies attract each other
under Newtonian gravity; their motion is integrated with velocity Verlet and
rendered live with [raylib](https://www.raylib.com/).

## Live demo

The simulator is automatically compiled to WebAssembly and deployed to
GitHub Pages on every push to `main`.  To enable this for your own fork:

1. Go to **Settings → Pages → Source** and choose **GitHub Actions**.
2. Push to `main` (or trigger the workflow manually via the Actions tab).
3. The page will be live at `https://<user>.github.io/<repo>/`.

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

### Web / WASM build

```sh
# 1. Build raylib for WebAssembly (once)
git clone --depth 1 --branch 5.0 https://github.com/raysan5/raylib.git
cd raylib/src
make PLATFORM=PLATFORM_WEB RAYLIB_LIBTYPE=STATIC RAYLIB_BUILD_MODE=RELEASE
mkdir -p ../../raylib-web/include ../../raylib-web/lib
cp raylib.h rlgl.h raymath.h ../../raylib-web/include/
cp libraylib.a ../../raylib-web/lib/
cd ../..

# 2. Build the simulator
make web RAYLIB_WEB=raylib-web

# 3. Serve locally (Python 3)
python3 -m http.server 8080 --directory web
# then open http://localhost:8080
```

The Makefile uses `pkg-config` to locate raylib when available and otherwise
links `-lraylib` directly. Build flags: `-Wall -Wextra -O2 -std=c11`.

## Controls

| Input            | Action                              |
| ---------------- | ----------------------------------- |
| Scroll wheel     | Zoom in / out (manual)              |
| Left-click drag  | Pan the view                        |
| `F`              | Toggle auto fit-to-view zoom        |
| `R`              | Reset the camera                    |
| `Space`          | Pause / resume                      |
| `1`              | Load solar system (default)         |
| `2`              | Load binary star + circumbinary planets |
| `3`              | Load random stellar cluster         |
| `A`              | Add a random body on a circular orbit |
| `D`              | Remove the most recently added body |
| `Esc`            | Quit                                |

The three built-in scenarios are:

- **Solar system** — Sun and all eight planets with real SI masses, radii, and
  mean circular-orbit velocities.
- **Binary star** — Two unequal stars (Alpha 2 M☉, Beta 1.5 M☉) orbiting their
  common centre of mass (~107-day period), plus two circumbinary planets
  (Kepler-b and Kepler-c) on stable outer orbits.
- **Random cluster** — A compact central mass (think nuclear star cluster)
  surrounded by 19 stellar-mass bodies in random circular orbits.

`A` and `D` work in any scenario; added bodies are placed on near-circular
orbits around the system's centre of mass.

By default the camera **auto-fits**: it continuously rescales and re-centers so
every body stays in frame as the system expands or collapses. Scrolling or
dragging hands control back to you (turning auto-fit off); press `F` to toggle
it again, or `R` to reset the view and re-enable it.

Bodies are drawn at a size that tracks both their mass and the current zoom, so
they shrink as you zoom out and grow as you zoom in. A map-style **scale bar** in
the bottom-right shows the on-screen distance for a round number of metres, km,
AU, or ly as you zoom across scales.

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
  main.c                 entry point: window loop, scene switching, add/remove bodies
  physics.h / physics.c  apply_grav(), velocity-Verlet integrate(), total_energy()
  render.h  / render.c   camera, draw_bodies(), draw_trails(), draw_hud()
  scenarios.h / scenarios.c  preset scenes (solar system, binary star, cluster)
                             and add_random_body()
  objects/
    obj_types.h          Vec3, Planetoid, Trail, SimCamera
    obj_types.c          vector helpers
    body.h / body.c      body_init/free, trail_init/push/free
web/
  shell.html             HTML page hosting the WebAssembly canvas
.github/workflows/
  deploy.yml             CI: builds WASM with Emscripten, deploys to GitHub Pages
```

## Physics notes

- Forces are summed pairwise (O(n²)) with a softening term `eps` to avoid
  singularities at very small separations.
- Integration is **velocity Verlet**, split around the force evaluation
  (half-kick → drift → recompute forces → half-kick). This symplectic scheme
  keeps total energy flat over long runs rather than letting orbits spiral.
- Units are SI (kilograms, metres, seconds) so `G = 6.674e-11` applies directly.
