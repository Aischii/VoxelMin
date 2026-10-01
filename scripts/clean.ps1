# Removes the build directory.
# Usage:  pwsh -File scripts/clean.ps1
$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $root "build"

if (Test-Path $buildDir) {
    Remove-Item -Recurse -Force $buildDir
    Write-Host "Removed $buildDir"
} else {
    Write-Host "Nothing to clean."
}
