# VoxelMin

[![CI](https://github.com/Aischii/VoxelMin/actions/workflows/ci.yml/badge.svg)](https://github.com/Aischii/VoxelMin/actions/workflows/ci.yml)

A minimal, single-player voxel sandbox inspired by early ("alpha") Minecraft,
written in C++17 with OpenGL 3.3 Core. The goal is a small, readable codebase
for **learning** and for **handing off between developers/AI agents**.

Everything visual is generated in code or drawn from a small 16x16 PNG texture
set; the shipped assets are the GLSL shader sources, the block textures, the
window icon, and the splash-text list.

---

## Status

**M5 "Gameplay & Entities" complete; current release v0.M5.5**. See
[`docs/PROGRESS.md`](docs/PROGRESS.md) for the living status -- including the
version history and the current performance metrics -- and
[`docs/ROADMAP.md`](docs/ROADMAP.md) for what comes next (opening M6: Audio & World Streaming).

## Features (current)

- Legacy-console-inspired UI: main menu with a rotating world panorama, an
  in-game pause menu, and an Options screen (keyboard/mouse navigable).
- Adjustable **GUI Scale** (Auto / 1-4) applied to menus and the HUD, with a
  safe bottom margin so text clears the taskbar in fullscreen.
- GLFW window + OpenGL 3.3 Core context.
- Procedural 5x7 pixel font (no font files).
- Chunked voxel world: 8x8 chunks, each 16 x 80 x 16 blocks.
- Seeded value-noise fBm terrain with five world types: `Default`, `Flat`,
  `Mountainous`, `Cavernous`, `Island`.
- Four randomised tree archetypes (classic oak, tall oak, conical spruce,
  bushy/apple) and 3D noise cave carving with coal, iron, gold and diamond veins.
- Water: sea-level basins, sandy beaches, swimming/diving physics, buoyancy,
  translucent transparent-pass rendering, and an underwater shader with dense
  blue fog and caustics.
- Lighting: skylight propagation with horizontal diffusion, smooth 4-level
  vertex ambient occlusion, and placeable torches with dynamic point-light
  propagation plus flame/smoke particles.
- 2D greedy meshing, frustum culling, dual-pass (opaque + alpha-cutout)
  rendering, and incremental chunk rebuilds.
- **Profiling**: an `F3` overlay with FPS, frame time, draw calls, triangles,
  chunk visible/drawn/culled counts, mobs and particles. Headless capture runs
  log the same numbers to stdout, so benchmarks are reproducible from a script.
- **Persistence**: named worlds in `saves/`, RLE-compressed binary saves, a
  Select World / Create New World menu with custom names, seeds and name entry,
  and auto-save on quit.
- **Player**: walking physics, creative flight, sprinting with dynamic FOV,
  swimming, 24-slot storage inventory with drag-and-drop and tooltips.
- **Mobs**: pigs and cows with an AI state machine (idle, wander, look-at-player,
  panic), voxel collision, animated legs, and batched dynamic rendering; left
  click deals knockback.
- 3D standing torch models, block breaking/placing via voxel ray marching, a
  selection outline, crosshair and hotbar UI.

## Requirements

- Windows with **MSYS2 + MinGW-w64** (GCC), CMake >= 3.20, Ninja.
- Libraries: GLFW, GLEW, GLM, OpenGL (all from the MSYS2 `mingw64` repo).

See [`docs/BUILD.md`](docs/BUILD.md) for the exact setup commands.

## Build & run

```powershell
pwsh -File scripts/build.ps1      # configure + build (Release)
pwsh -File scripts/run.ps1        # run the game
pwsh -File scripts/build.ps1 -Debug   # debug build
pwsh -File scripts/clean.ps1      # remove build/
```

The executable is written to `build/bin/VoxelMin.exe` and is **self-contained**
(the GCC runtime is linked statically; assets are copied beside it).

## Controls

The game opens on the **main menu**. All menus can be navigated with the
keyboard, the mouse, or both.

**Menus**

| Input | Action |
| --- | --- |
| `Up` / `Down` or `W` / `S` | Move selection |
| `Left` / `Right` or `A` / `D` | Change an option value |
| `Enter` / `Space` | Confirm |
| Mouse move / click | Hover and activate |
| `Esc` | Back (pause menu) / close options |

**In game**

| Input | Action |
| --- | --- |
| `W` `A` `S` `D` | Move |
| Mouse | Look |
| `Left Ctrl` / double-tap `W` | Sprint (dynamic FOV) |
| `Space` | Jump / fly up / swim up |
| `Left Shift` | Fly down / dive |
| `F` | Toggle fly / walk |
| `E` | Open / close the storage inventory |
| `1`..`8` / scroll | Select block |
| Left click | Break block / hit mob |
| Right click | Place block |
| `G` | Toggle wireframe debug view |
| `F3` | Toggle the profiling overlay |
| `Esc` | Open the in-game menu |
| Window close button | Quit |

## Project layout

```
VoxelMin/
├── CMakeLists.txt          Build definition
├── README.md               This file
├── AGENTS.md               Quick guide for AI agents / new contributors
├── assets/shaders/         GLSL sources (chunk, line, ui, text, sprite, particle, underwater)
├── assets/textures/        16x16 PNG block textures
├── saves/                  Named world saves (created at runtime)
├── docs/                   Architecture, progress, roadmap, handoff
├── scripts/                build / run / clean PowerShell scripts
└── src/
    ├── main.cpp            Entry point
    ├── core/               Application (loop + state machine), config, logging
    ├── input/              Polled input state
    ├── player/             Player physics + camera control
    ├── entity/             Mob models, AI state machine, entity manager
    ├── render/             Shaders, mesh, texture atlas, bitmap font, camera,
    │                       particles, frustum culling, renderer
    ├── ui/                 Menu model + Legacy-style menu rendering
    └── world/              Blocks, chunk, world, generation, meshing, raycast,
                            save/load
```

## Documentation

- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) -- modules, data flow, coordinate systems.
- [`docs/PROGRESS.md`](docs/PROGRESS.md) -- what works, what is broken, what is next.
- [`docs/ROADMAP.md`](docs/ROADMAP.md) -- planned milestones.
- [`docs/HANDOFF.md`](docs/HANDOFF.md) -- how to continue the project (read this first when taking over).
- [`docs/BUILD.md`](docs/BUILD.md) -- toolchain setup and troubleshooting.
- [`AGENTS.md`](AGENTS.md) -- condensed instructions for automated agents.
