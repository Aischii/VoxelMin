# AGENTS.md

Condensed operating guide for AI agents and new contributors. Read
`docs/HANDOFF.md` and `docs/ARCHITECTURE.md` for the full picture.

## Build / run / verify

### Linux
```bash
./scripts/build.sh          # configure + build (Release)
./scripts/run.sh            # launch
./scripts/build.sh --debug  # debug build
./scripts/clean.sh          # wipe build/
```

### Windows
```powershell
pwsh -File scripts/build.ps1        # configure + build (Release)
pwsh -File scripts/run.ps1          # launch
pwsh -File scripts/build.ps1 -Debug # debug build
pwsh -File scripts/clean.ps1        # wipe build/
```

- Build must finish with **zero warnings** (`-Wall -Wextra -Wpedantic`).
- There is no automated test suite yet. "Verify" means: build clean, then run
  and confirm the startup logs appear and the window stays up:
  `Renderer initialised (OpenGL 3.3.0 ...)` / `VoxelMin v0.M5.4 starting...` / `World ready: 128 x 80 x 128 blocks (voxel RAM: ... KiB)`.
- The build output `build/bin/VoxelMin` (or `.exe` on Windows) is self-contained;
  run it from `build/bin` so `assets/` resolves (the run scripts do this).

## Toolchain

- **Linux**: GCC or Clang with CMake and Ninja/Make.
  - Fedora: `sudo dnf install gcc-c++ cmake ninja-build glfw-devel glew-devel glm-devel stb-devel`
  - Ubuntu/Debian: `sudo apt install g++ cmake ninja-build libglfw3-dev libglew-dev libglm-dev libstb-dev`
  - Arch: `sudo pacman -S gcc cmake ninja glfw-x11 glew glm stb`
- **Windows**: MSYS2 MinGW-w64. Configure and build with the `mingw64` compiler; CMake finds
  GLFW/GLEW/GLM/stb from the prefix next to the compiler. If a tool is missing:

```powershell
winget install MSYS2.MSYS2
C:\msys64\usr\bin\pacman.exe -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake `
    mingw-w64-x86_64-ninja mingw-w64-x86_64-glfw mingw-w64-x86_64-glm mingw-w64-x86_64-glew mingw-w64-x86_64-stb
```

## Code map

| Path | Responsibility |
| --- | --- |
| `src/core/Application.*` | Window, main loop, game state machine, menus, GLFW callbacks |
| `src/core/Config.hpp` | All tunable constants (world size, speeds, fog, seed) |
| `src/input/Input.*` | Polled keyboard/mouse state with per-frame edge detection |
| `src/player/Player.*` | Movement, gravity, collision, camera owner |
| `src/render/Renderer.*` | Draws world, selection outline, HUD, 2D primitives, text, F3 overlay |
| `src/render/FrameStats.*` | Per-frame draw/triangle/chunk counters + 90-frame rolling average (F3) |
| `src/render/Frustum.hpp` | 6-plane Gribb-Hartmann view frustum chunk AABB culler |
| `src/render/Font.*` `FontData.hpp` | Procedural 5x7 bitmap font (glyph table + atlas) |
| `src/render/Shader.*` `Mesh.*` `Texture.*` `Camera.*` | GL/math wrappers & HD texture loader |
| `src/ui/Menu.*` | Menu model + Legacy-console-style menu rendering |
| `src/world/Block.hpp` | Block ids, face->tile mapping, solids, hotbar colours |
| `src/entity/MobAi.hpp` | Mob type, named AI goals + per-species goal masks, AI tuning |
| `src/entity/Mob.*` | Mob physics, goal-driven movement (step-up/jump/ledge), models |
| `src/entity/EntityManager.*` | Mob list, spawning, entity raycast hit test, batched meshes |
| `src/world/Chunk.*` | WxHxD voxel storage + GPU mesh + dirty flag |
| `src/world/World.*` | Chunk grid, world-space block get/set |
| `src/world/WorldSave.*` | Binary world persistence (save/load `world.dat` + RLE compression) |
| `src/world/TerrainGenerator.*` | Seeded noise terrain + trees |
| `src/world/ChunkMesher.*` | 2D Greedy meshing & dual-pass geometry builder |
| `src/world/Raycast.*` | Voxel DDA for block picking |
| `src/main.cpp` | Entry point |

## Conventions / invariants

- C++17, `namespace vox`, `m_` prefix for members, 4-space indent.
- Coordinates: world starts at `(0,0,0)` and grows in +X/+Z only (no negative
  world coords). `y < 0` in `World::getBlock` returns Bedrock so bottom faces
  are culled.
- Chunk geometry is baked in **world space**; there is no model matrix for chunks.
- Faces are emitted only when the neighbour is non-opaque. Culling uses
  `isOpaque()`, so transparent blocks would need a separate pass.
- `Input::newFrame()` must be called once per frame *before* `glfwPollEvents()`.
- Meshing is incremental: `Chunk::dirty` is set on edits (including neighbour
  chunks at borders) and `Application::rebuildDirtyMeshes()` rebuilds a few per
  frame.
- The game starts in `GameState::MainMenu`. Only `Playing` updates the player
  and interaction; menus run their own input path (`handleMenuInput`).
- UI/menu drawing uses `Renderer` 2D primitives (pixels, origin bottom-left).
  Call `beginUI()`/`endUI()` around it (blend on, depth/cull off).

## Screenshot capture (for verifying rendering)

Headless capture is built in and driven by environment variables:

```powershell
$env:VOXELMIN_CAPTURE="out.bmp"          # required: output path
$env:VOXELMIN_CAPTURE_STATE="menu"       # menu|play|pause|options|pause_options
$env:VOXELMIN_CAPTURE_FRAMES="120"       # optional, default 90
$env:VOXELMIN_CAPTURE_SIZE="1600x900"    # optional: resize window before capture
$env:VOXELMIN_CAPTURE_GUI_SCALE="4"      # optional: force GUI scale (0 = auto)
$env:VOXELMIN_CAPTURE_DEBUG="1"          # optional: force the F3 overlay on
$env:VOXELMIN_CAPTURE_VSYNC="0"          # optional: drop vsync for perf numbers
build\bin\VoxelMin.exe
```

It writes a BMP and exits. Convert to PNG with
`[System.Drawing.Bitmap]::FromFile(...).Save(...,'Png')` to inspect.

`VOXELMIN_CAPTURE_SIZE` resizes the *window*, so the captured framebuffer is
slightly shorter than requested (Windows subtracts the title bar and border --
e.g. `1920x1080` yields a `1920x1061` framebuffer).

Every capture also logs a `Stats:` line to stdout with the rolling averages
(frame time, draw calls, triangles, chunks visible/drawn/culled, mobs,
particles). Combine with `VOXELMIN_CAPTURE_VSYNC=0` to measure the engine
rather than the monitor refresh rate. Frame time is CPU-side only -- OpenGL
submission is async, so GPU time is not included.

## Common tasks

**Add a block type**
1. Add the enum value in `BlockId` and a tile in `TextureTile` (`Block.hpp`).
2. Add its entry to the `defs[]` table.
3. Add a generator in `render/Texture.cpp` (`createAtlas`) and a `blockColor`.
4. Optionally add it to `Application::m_hotbar`.
No meshing/rendering changes are needed.

**Change world size / seed** -- edit `src/core/Config.hpp`.

## Gotchas

- `glewExperimental = true` then `glewInit()` leaves one benign `GL_INVALID_ENUM`;
  `Application::init()` clears it with `glGetError()`.
- PowerShell does not expand `$var` reliably in an unquoted `-DNAME=$var` native
  argument; `scripts/build.ps1` uses an args array instead. Keep it that way.
- Diagnostics are `printf`-based (`core/Log.hpp`) and stdout is unbuffered.
