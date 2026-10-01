param(
    [string]$Output = "capture.bmp",
    [string]$State = "play",
    [int]$Frames = 100,
    [string]$Size = "1280x720"
)

$env:VOXELMIN_CAPTURE = $Output
$env:VOXELMIN_CAPTURE_STATE = $State
$env:VOXELMIN_CAPTURE_FRAMES = $Frames
$env:VOXELMIN_CAPTURE_SIZE = $Size
$exe = "build/bin/VoxelMin"
if (-not (Test-Path $exe)) {
    $exe = "build\bin\VoxelMin.exe"
}

& $exe

$pngName = [System.IO.Path]::ChangeExtension($Output, ".png")
if (Test-Path $Output) {
    Add-Type -AssemblyName System.Drawing
    $bmp = [System.Drawing.Bitmap]::FromFile((Resolve-Path $Output).Path)
    $bmp.Save((Join-Path (Get-Location) $pngName), [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Host "Saved screenshot to $pngName"
}
