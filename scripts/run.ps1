# Runs the built game from its output directory (so assets resolve).
# Usage:  pwsh -File scripts/run.ps1
$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $root "build\bin\VoxelMin.exe"
if (-not (Test-Path $exe)) {
    $exe = Join-Path $root "build/bin/VoxelMin"
}

if (-not (Test-Path $exe)) {
    throw "Executable not found at $exe. Build first: pwsh -File scripts/build.ps1 or ./scripts/build.sh"
}

Push-Location (Split-Path -Parent $exe)
try {
    & $exe
} finally {
    Pop-Location
}
