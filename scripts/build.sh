#!/usr/bin/env bash
# Builds VoxelMin on Linux.
# Usage:  ./scripts/build.sh
#         ./scripts/build.sh --debug

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

# If CMakeCache.txt was generated on another machine or OS (e.g. copied from Windows), clean it.
if [[ -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
    if grep -q "CMAKE_CACHEFILE_DIR" "${BUILD_DIR}/CMakeCache.txt"; then
        CACHE_DIR=$(grep "^CMAKE_CACHEFILE_DIR:INTERNAL=" "${BUILD_DIR}/CMakeCache.txt" | cut -d= -f2- || true)
        if [[ -n "${CACHE_DIR}" && "${CACHE_DIR}" != "${BUILD_DIR}" ]]; then
            echo "Detected mismatched CMakeCache.txt (built at ${CACHE_DIR}). Cleaning build directory..."
            rm -rf "${BUILD_DIR}"
        fi
    fi
fi

BUILD_TYPE="Release"
for arg in "$@"; do
    case "$arg" in
        -d|--debug|Debug)
            BUILD_TYPE="Debug"
            ;;
        *)
            ;;
    esac
done

GENERATOR_ARGS=()
if command -v ninja >/dev/null 2>&1; then
    GENERATOR_ARGS=("-G" "Ninja")
fi

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" "${GENERATOR_ARGS[@]}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
cmake --build "${BUILD_DIR}"

if [[ -f "${BUILD_DIR}/compile_commands.json" ]]; then
    cp -f "${BUILD_DIR}/compile_commands.json" "${ROOT_DIR}/compile_commands.json"
fi

echo "Build succeeded (${BUILD_TYPE}). Run it with: ./scripts/run.sh"
