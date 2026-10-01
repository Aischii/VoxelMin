# Build & toolchain

## Prerequisites

- Windows 10/11 or Linux (Fedora, Ubuntu/Debian, Arch, etc.).
- [MSYS2](https://www.msys2.org/) (`winget install MSYS2.MSYS2`) on Windows.
- CMake >= 3.20 and Ninja or Make. The floor is 3.20; development and CI run on **CMake 4.4**, which is what the `POST_BUILD` `copy_directory` behaviour is verified against.
- C++17 is pinned by `set(CMAKE_CXX_STANDARD 17)` in `CMakeLists.txt`.

## One-time setup

### Windows (MSYS2 MinGW-w64)

```powershell
# From a normal PowerShell prompt:
C:\msys64\usr\bin\pacman.exe -S --needed `
    mingw-w64-x86_64-gcc `
    mingw-w64-x86_64-cmake `
    mingw-w64-x86_64-ninja `
    mingw-w64-x86_64-glfw `
    mingw-w64-x86_64-glew `
    mingw-w64-x86_64-glm `
    mingw-w64-x86_64-stb
```

The `mingw64` prefix (default `C:\msys64\mingw64`) contains the compiler,
`include/` (GLFW, GLM, GL, stb), `lib/` (GLFW, GLEW) and its own `bin/`.

### Linux

**Fedora:**
```bash
sudo dnf install gcc-c++ cmake ninja-build glfw-devel glew-devel glm-devel stb-devel
```

**Ubuntu / Debian:**
```bash
sudo apt update && sudo apt install g++ cmake ninja-build libglfw3-dev libglew-dev libglm-dev libstb-dev
```

**Arch Linux:**
```bash
sudo pacman -S --needed gcc cmake ninja glfw-x11 glew glm stb
```

## Build

### Linux
```bash
./scripts/build.sh          # Release
./scripts/build.sh --debug  # Debug
```

### Windows (PowerShell)
```powershell
pwsh -File scripts/build.ps1          # Release
pwsh -File scripts/build.ps1 -Debug   # Debug
```

Equivalent manual commands:

```bash
# Linux
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

```powershell
# Windows
$env:PATH = "C:\msys64\mingw64\bin;$env:PATH"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Output: `build/bin/VoxelMin` (Linux) or `build/bin/VoxelMin.exe` (Windows) (self-contained; `assets/` is copied beside it).

## Asset deployment

A `POST_BUILD` custom command in `CMakeLists.txt` runs
`cmake -E copy_directory` and copies the whole `assets/` tree next to the
executable, so shaders and textures resolve relative to the exe. This is a
directory copy, not a per-file list: **any file you add under `assets/` is
picked up automatically** and you do not need to edit the build.

To verify it ran:

```powershell
Test-Path build\bin\assets\shaders\chunk.vert   # expect True
Test-Path build\bin\assets\textures\stone.png  # expect True
```

Run the exe from `build/bin` (or via `scripts/run.ps1`) -- launching it from
another directory breaks relative asset paths.

Sources are globbed with `GLOB_RECURSE ... CONFIGURE_DEPENDS`, so a new `.cpp`
under `src/` is also picked up without editing `CMakeLists.txt`; Ninja re-runs
the glob check on each build.

## IDE & compiler database

This is the canonical description of the IDE setup; the other docs link here
rather than restating it.

- **VS Code**: `.vscode/c_cpp_properties.json` points at MSYS2 include paths
  (`C:/msys64/mingw64/include`) and `${workspaceFolder}/compile_commands.json`.
- **clangd**: `.clangd` configures system include paths and points at the same
  compilation database.
- `scripts/build.ps1` copies `build/compile_commands.json` to the repo root after
  every build, so the database is never stale. **Keep headers self-contained**
  (each header includes what it uses) so the language server can resolve types.

## Run

### Linux
```bash
./scripts/run.sh
```

### Windows
```powershell
pwsh -File scripts/run.ps1
```

The script changes into `build/bin` so the shaders resolve through the
`assets/shaders/...` relative path. `Renderer::resolveAsset` also tries
`../assets` and `../../assets`, so running from the repo root works too.

## Clean

### Linux
```bash
./scripts/clean.sh
```

### Windows
```powershell
pwsh -File scripts/clean.ps1
```

## Troubleshooting

| Symptom | Cause / fix |
| --- | --- |
| `Could not find GLFW/GLEW` at configure | Install the pacman packages above, or point `HINTS` in `CMakeLists.txt` at your prefix. |
| `ninja: error: ... expected newline, got lexing error` with `$buildType` in the rule name | PowerShell did not expand the variable. Ensure `scripts/build.ps1` still uses the `$configureArgs` array. |
| Exe exits with code `-1073741515` (`0xC0000135`) | Missing DLL. The build links `-static`; if you removed it, either restore it or add `C:\msys64\mingw64\bin` to `PATH`. |
| Window is black | A shader failed to compile (check stdout) or the atlas was not generated. |
| `GL_INVALID_ENUM` logged once at startup | Expected - GLEW's known artifact; `Application::init()` clears it. |
| Blocks render pink, black, or as flat noise | The PNG failed to decode, so the tile fell back to its procedural generator. Check stdout for the `stbi_load` error and confirm `build\bin\assets\textures\` is populated. Note 7 tiles currently have no PNG at all (ores, cobblestone, torch, water) and are *expected* to look procedural. |
| Shaders fail to compile, or `#version` errors | `assets/shaders/` was not copied next to the exe. Check `build\bin\assets\shaders\chunk.vert` exists; if not, the POST_BUILD copy did not run -- rebuild. |
| Everything is one flat colour / no geometry | Usually the world failed to generate or the save did not load; check the startup log for `World ready:`. |
| Save file will not load, "unknown magic" | VOXS version mismatch. v2 reads v1 and v2; anything else is rejected with a log line. A save from a newer build is not readable by an older one. |
