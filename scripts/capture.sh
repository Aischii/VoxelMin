#!/usr/bin/env bash
# Headless capture helper for VoxelMin on Linux.
# Usage: ./scripts/capture.sh [output.bmp] [state] [frames] [size]

set -euo pipefail

OUTPUT="${1:-capture.bmp}"
STATE="${2:-play}"
FRAMES="${3:-100}"
SIZE="${4:-1280x720}"

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN_DIR="${ROOT_DIR}/build/bin"
EXE="${BIN_DIR}/VoxelMin"

if [[ ! -f "${EXE}" ]]; then
    if [[ -f "${EXE}.exe" ]]; then
        EXE="${EXE}.exe"
    else
        echo "Error: Executable not found at ${EXE}." >&2
        echo "Build first with: ./scripts/build.sh" >&2
        exit 1
    fi
fi

export VOXELMIN_CAPTURE="${OUTPUT}"
export VOXELMIN_CAPTURE_STATE="${STATE}"
export VOXELMIN_CAPTURE_FRAMES="${FRAMES}"
export VOXELMIN_CAPTURE_SIZE="${SIZE}"

"${EXE}"

PNG_NAME="${OUTPUT%.*}.png"
if [[ -f "${OUTPUT}" ]]; then
    if command -v magick >/dev/null 2>&1; then
        magick "${OUTPUT}" "${PNG_NAME}"
        echo "Saved screenshot to ${PNG_NAME}"
    elif command -v convert >/dev/null 2>&1; then
        convert "${OUTPUT}" "${PNG_NAME}"
        echo "Saved screenshot to ${PNG_NAME}"
    elif command -v python3 >/dev/null 2>&1 && python3 -c "import PIL.Image" >/dev/null 2>&1; then
        python3 -c "from PIL import Image; Image.open('${OUTPUT}').save('${PNG_NAME}')"
        echo "Saved screenshot to ${PNG_NAME}"
    else
        echo "Saved capture to ${OUTPUT} (install ImageMagick or python-pillow to auto-convert to PNG)"
    fi
fi
