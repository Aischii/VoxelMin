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

**M7 "Lighting, Visual Immersion & Inventory Evolution" active; current release v0.M7.0**. See
[`docs/PROGRESS.md`](docs/PROGRESS.md) for the living status -- including the
version history and current performance metrics -- and
[`docs/ROADMAP.md`](docs/ROADMAP.md) for the project roadmap.

## Features (current)

- **Atmospheric Lighting & Day/Night**: Dual-channel 4-bit Sunlight + 4-bit Blocklight nibble storage in chunks with BFS flood-fill propagation. Smooth 4-corner vertex AO with diagonal flipping. Real pitch-black darkness at night and in deep unlit caverns with celestial sun and moon orbits.
- **Separate Menu & In-Game Music**: Dedicated procedural arpeggio title theme for the main menu and mystical ambient pad exploration music in-game, with seamless dynamic crossfading and external file scanning.
- **Distinct Survival vs Creative Inventories**:
  - *Survival*: 2x2 Crafting Grid + Result slot, 24-slot backpack, 8-slot hotbar, durability bars, and drag/drop stack management.
  - *Creative*: Comprehensive Item Catalog with Category Filter Tabs (`All Items`, `Blocks`, `Tools`, `Items & Food`), infinite item generation, quick number key slot copying, and a dedicated trash slot.
- **Minecraft-Style Block Breaking**: Real 10-stage cracking texture overlays (`destroy_stage_0.png` to `destroy_stage_9.png`) mapped cleanly over targeted voxels.
- **Future-Proof Persistence**: Versioned binary world saves with header magic, compression metadata, chunk tables, and backward/forward save migration.
- **Procedural Villages & Structures**: Wells, houses, towers, and lamp posts naturally generated on surface terrain.
- **Full Sound Effects Suite**: Procedural digging, block placement, tool breaks, footsteps, damage grunts, eating, burping, and item pickups.
- **RPG Survival Vitals**: Health, hunger, oxygen with underwater drowning, and screen-edge horror hurt vignettes.
- **Legacy-Console UI**: Main menu panorama, pause menu, custom world creation, options screen with adjustable GUI Scale (Auto / 1-4).
- **Profiling & Tools**: `F3` debug overlay with draw calls, FPS, frame times, particle/chunk counts, and headless capture support.

## Requirements

- **Linux**: GCC/Clang, CMake >= 3.20, Ninja/Make, `libglfw3-dev`, `libglew-dev`, `libglm-dev`, `libstb-dev`.
- **Windows**: MSYS2 with `mingw-w64-x86_64-gcc`, `mingw-w64-x86_64-cmake`, `mingw-w64-x86_64-ninja`, `mingw-w64-x86_64-glfw`, `mingw-w64-x86_64-glm`, `mingw-w64-x86_64-glew`, `mingw-w64-x86_64-stb`.

See [`docs/BUILD.md`](docs/BUILD.md) for the exact setup commands.

## Build & run

### Linux
```bash
./scripts/build.sh          # configure + build (Release)
./scripts/run.sh            # run the game
./scripts/build.sh --debug  # debug build
./scripts/clean.sh          # remove build/
```

### Windows
```powershell
pwsh -File scripts/build.ps1        # configure + build (Release)
pwsh -File scripts/run.ps1          # run the game
pwsh -File scripts/build.ps1 -Debug # debug build
pwsh -File scripts/clean.ps1        # remove build/
```

## Controls

The game opens on the **main menu**. All menus can be navigated with the keyboard, the mouse, or both.

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
| `F4` / `C` | Toggle Creative / Survival mode |
| `F5` | Cycle perspective (1st person / 3rd person) |
| `E` | Open / close inventory (Survival crafting or Creative catalog) |
| `1`..`8` / scroll | Select hotbar slot |
| Left click | Hold to mine block / attack mob / pick creative stack |
| Right click | Place block / eat food / pick single creative item |
| `G` | Toggle wireframe debug view |
| `F3` | Toggle profiling overlay |
| `Esc` | Pause menu |

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
