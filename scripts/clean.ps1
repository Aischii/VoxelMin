# Removes the build directory.
# Usage:  pwsh -File scripts/clean.ps1
$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $root "build"

if (Test-Path $buildDir) {
    $binSaves = Join-Path $buildDir "bin\saves"
    $rootSaves = Join-Path $root "saves"
    if (Test-Path $binSaves) {
        if (!(Test-Path $rootSaves)) { New-Item -ItemType Directory -Path $rootSaves | Out-Null }
        Copy-Item -Path "$binSaves\*" -Destination $rootSaves -Recurse -Force -ErrorAction SilentlyContinue
    }
    Remove-Item -Recurse -Force $buildDir
    Write-Host "Removed $buildDir (Saves preserved in $rootSaves)"
} else {
    Write-Host "Nothing to clean."
}
