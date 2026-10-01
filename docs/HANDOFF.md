# Handoff

You are picking up VoxelMin, a small C++17 / OpenGL 3.3 voxel game (the standard
is pinned by `set(CMAKE_CXX_STANDARD 17)` in `CMakeLists.txt`; do not use C++20
features). This file tells you what to read, how to verify your changes, and
which decisions were already made so you do not accidentally undo them.

## TL;DR for a new agent

1. Read `README.md`, then `docs/ARCHITECTURE.md`, then `docs/PROGRESS.md`.
2. Build: `pwsh -File scripts/build.ps1` (must be warning-free).
3. Run: `pwsh -File scripts/run.ps1` and confirm the logs in
   `docs/PROGRESS.md` -> "Verification checklist".
4. Pick the next unstarted item from `docs/ROADMAP.md`. For the current release
   and what is left in the current milestone, read the top of
   `docs/PROGRESS.md`. **This file tells you how to work here; PROGRESS.md is
   the single source of state.** Do not copy the current version or milestone
   into this file -- it will go stale.
5. When done: build clean, run-verify, and **update `docs/PROGRESS.md`** with a
   new dated entry plus any new limitations. Update `docs/ARCHITECTURE.md` if you
   changed the design.

## Environment assumptions

- Windows + MSYS2 MinGW-w64. The compiler, CMake, Ninja, GLFW, GLEW and GLM
  live under `C:\msys64\mingw64` (or wherever MSYS2 was installed). `scripts`
  prepend that `bin` to `PATH`.
- If MSYS2 is somewhere else, edit `scripts/build.ps1` (`$mingw`). The CMake
  configure discovers libraries relative to the compiler, so no other edit is
  needed.
- Do **not** commit `build/` (see `.gitignore`).

## Decisions already made (and why)

- **Static linking** (`-static`): earlier the exe failed with
  `STATUS_DLL_NOT_FOUND` because MinGW links `libstdc++`/`libwinpthread`
  dynamically. Static linking keeps the build self-contained. Keep it.
- **`GLEW_STATIC`** is defined in CMake; the loader is linked statically.
- **Image assets exist** (`assets/textures/*.png`, `assets/icon.png`, GLSL under
  `assets/shaders/`). Early builds generated every texture procedurally;
  `Texture::createAtlas()` now decodes PNGs with `stbi_load` and rescales with
  `stbir_resize_uint8_linear`, but **keeps a procedural fallback per tile**, so a
  missing or corrupt PNG degrades that one block instead of breaking startup.
  If you add a new asset type, document where it is loaded from.
- **World starts at (0,0,0), grows into +X/+Z only.** This avoids negative
  floor-division bugs. If you add infinite streaming you will need proper
  floor division for negative chunk coordinates -- handle it deliberately.
- **Chunk geometry baked in world space**, so there is no per-chunk model matrix.
- **Face winding** is derived from the `(normal, tangent, bitangent)` table in
  `src/world/ChunkMesher.cpp`, requiring `t x b == n`. If you touch that table,
  re-check winding or faces will be culled incorrectly.
- **`Input::newFrame()` before `glfwPollEvents()`** -- this ordering is load
  bearing for edge detection.
- **Menus are data + lambdas in `Application::buildMenus()`.** Option rows both
  read and write runtime settings and call `applySettings()`; keep that the
  single place settings are pushed to the player/camera/window.
- **Cursor capture happens only via `Application::setCursorCaptured()`** so the
  GLFW cursor mode and `Input` never disagree.
- **The bitmap font is hand-authored** in `src/render/FontData.hpp` (5x7, one
  line per glyph). Lowercase and punctuation exist; if you add unicode, extend
  `Font::build()` beyond the ASCII lookup table.
- **IDE/compiler-database setup** is documented once, in
  [`BUILD.md`](BUILD.md) -> "IDE & compiler database". The short version: keep
  headers self-contained so the language server resolves types, and run the
  build script (it refreshes the root `compile_commands.json`).
- **Build script uses an args array** for the CMake configure call. PowerShell
  does not expand `$var` in an unquoted `-DNAME=$var` native argument, producing
  a literal `$buildType` in `CMakeCache.txt` and breaking Ninja. Do not
  "simplify" it back to a single line.

## How to work here

- Follow existing style: C++17, `namespace vox`, `m_` member prefix, 4-space
  indent, no comments unless they explain *why*.
- Keep tunables in `src/core/Config.hpp`.
- Adding a block is a 4-step, non-invasive recipe -- see `AGENTS.md`.
- The renderer is intentionally thin; gameplay logic does not belong in it.

## Verifying rendering changes

The build has a built-in, environment-driven screenshot capture. It renders a
chosen game state, writes a BMP to the path in `VOXELMIN_CAPTURE` and exits:

```powershell
$env:VOXELMIN_CAPTURE="menu.bmp"
$env:VOXELMIN_CAPTURE_STATE="menu"   # menu|play|pause|options|pause_options
build\bin\VoxelMin.exe
```

Convert the BMP to PNG to view it (Windows has System.Drawing):

```powershell
Add-Type -AssemblyName System.Drawing
$b = [System.Drawing.Bitmap]::FromFile("menu.bmp"); $b.Save("menu.png",'Png'); $b.Dispose()
```

For menus, this is the fastest way to check layout without clicking. For a plain
crash check, launch it and confirm the startup logs (`Log.hpp` writes to
unbuffered stdout) mention the renderer, font and world.

Any GL error you introduce will usually surface as a blank/black window or a
crash; keep `glGetError()` checks handy when debugging.

## Measuring performance

Press `F3` in-game (works in menus too) for a rolling-average overlay: frame
time, draw calls, triangles, chunks visible/drawn/culled, mobs and particles.

For a headless benchmark, every capture already logs a `Stats:` line with the
same numbers, so it can be scripted:

```powershell
$env:VOXELMIN_CAPTURE="m.bmp"; $env:VOXELMIN_CAPTURE_STATE="play"
$env:VOXELMIN_CAPTURE_FRAMES="500"; $env:VOXELMIN_CAPTURE_VSYNC="0"
build\bin\VoxelMin.exe
```

Two caveats, both of which have bitten this project before:

- **Vsync must be off** to measure the engine. The dev display is 165 Hz, so a
  vsync-on run reports 6.06 ms no matter how fast the game actually is.
- **Frame time is CPU-side only.** GL submission is async, so these numbers
  exclude GPU cost. To measure GPU time you need `glFinish()` around the frame
  or `GL_TIME_ELAPSED` timer queries -- still an open M7 item.

The current baseline lives in the `Current metrics` table in `docs/PROGRESS.md`.
Re-measure after any renderer or worldgen change; M6 chunk streaming is the next
thing that will move these numbers.

## Open questions

**Do not keep a list here.** Open decisions live in exactly one place:
[`PROGRESS.md`](PROGRESS.md) -> "Open decisions (blocking M6)". Two copies
drift, and this file is read for *how to work here*, not for current state.

If you discover a new open decision, add it to that PROGRESS section. If a
question is really about mechanics rather than an upcoming milestone, it belongs
in the `Deferred` or `Backlog` list in [`ROADMAP.md`](ROADMAP.md).
