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

**M8 "Console Crafting, RPG HUD & Ecological Survival" active; current release v0.M8.0**. See
[`docs/PROGRESS.md`](docs/PROGRESS.md) for the living status -- including the
version history and current performance metrics -- and
[`docs/ROADMAP.md`](docs/ROADMAP.md) for the project roadmap.

## Features (current)

- **Atmospheric Lighting & 8-Phase Moon Cycle**: Dual-channel 4-bit Sunlight + 4-bit Blocklight nibble storage in chunks with BFS flood-fill propagation. Smooth 4-corner vertex AO with diagonal flipping. 8 lunar phases (Full Moon soft glow down to pitch-black New Moon darkness) and dynamic celestial orbits.
- **Console Edition Crafting UI & Interactive Workbench**: Authentic Legacy Console Edition crafting interface with 5 category tabs (`Structures`, `Tools & Weapons`, `Food & Essentials`, `Mechanisms`, `Decorations`), horizontal recipe selector, visual ingredient preview, yield slots, and 3x3 Crafting Table interaction.
- **RPG MMO Vitals Card**: Top-left player card displaying player portrait, Level badge, vertical XP meter, player name, red health bar, and Food, Oxygen, and Armor stats with sub-stat counters, dynamically scaled to window dimensions.
- **Living World Ecology**: Natural dirt-to-grass spreading under open sunlight, unsupported tall grass collapse when base block is mined, and shovel grass-to-dirt-path tilling.
- **First-Person Viewmodel & 3D Held Items**: Camera-locked right arm rendering with dynamic item holding (3D mini-blocks and 2.5D tool sprites) with smooth walk bobbing and swing animations.
- **Enhanced Physics, Item Toss & Double-Jump Flight**: Despawning item entities with 1.6m magnetic draw, 'Q' single item toss, Ctrl+'Q' full stack toss with impulse momentum, and double-space creative flight toggle.
- **Non-Freezing Game Over**: Continuous world simulation during the Game Over screen (mobs wander, items bob, particles drift).
- **Separate Menu & In-Game Music**: Dedicated procedural arpeggio title theme for the main menu and mystical ambient pad exploration music in-game, with seamless dynamic crossfading and external file scanning.
- **Minecraft-Style Block Breaking**: Real 10-stage cracking texture overlays (`destroy_stage_0.png` to `destroy_stage_9.png`) mapped cleanly over targeted voxels.
- **Future-Proof Persistence**: Versioned binary world saves with header magic, multi-path save deletion, compression metadata, chunk tables, and backward/forward save migration.
- **Procedural Villages & Structures**: Wells, houses, towers, and lamp posts naturally generated on surface terrain with resident pigman villagers.
- **Full Sound Effects Suite**: Procedural digging, block placement, tool breaks, footsteps, damage grunts, eating, burping, and item pickups.

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
| `Space` | Jump / swim up / double-tap to fly (Creative) |
| `Left Shift` | Fly down / dive |
| `Q` / `Ctrl+Q` | Drop 1 item / drop full stack |
| `F4` / `C` | Toggle Creative / Survival mode |
| `F5` | Cycle perspective (1st person / 3rd person) |
| `E` | Open / close inventory & crafting |
| `1`..`8` / scroll | Select hotbar slot |
| Left click | Hold to mine block / attack mob / pick creative stack |
| Right click | Place block / eat food / interact Crafting Table |
| `G` | Toggle wireframe debug view |
| `F3` | Toggle profiling overlay |
| `Esc` | Pause menu / close dialogs |

## Project layout

```
VoxelMin/
├── CMakeLists.txt          Build definition
├── README.md               This file
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
