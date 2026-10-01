# Builds VoxelMin with the MSYS2 MinGW-w64 toolchain.
# Usage:  pwsh -File scripts/build.ps1
#         pwsh -File scripts/build.ps1 -Debug
param(
    [switch]$Debug
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$mingw = "C:\msys64\mingw64\bin"
if (Test-Path $mingw) {
    $env:PATH = "$mingw;$env:PATH"
} else {
    Write-Warning "MSYS2 mingw64 not found at $mingw; relying on PATH."
}

$buildType = if ($Debug) { "Debug" } else { "Release" }
$buildDir = Join-Path $root "build"

$configureArgs = @(
    "-S", $root,
    "-B", $buildDir,
    "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=$buildType"
)
cmake @configureArgs
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

cmake --build $buildDir
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

if (Test-Path "$buildDir/compile_commands.json") {
    Copy-Item "$buildDir/compile_commands.json" "$root/compile_commands.json" -Force
}

Write-Host "Build succeeded ($buildType). Run it with: pwsh -File scripts/run.ps1"
