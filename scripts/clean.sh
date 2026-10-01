#!/usr/bin/env bash
# Removes the build directory.
# Usage:  ./scripts/clean.sh

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

if [[ -d "${BUILD_DIR}" ]]; then
    # Future-proof save preservation: backup any saves from build/bin/saves to root saves/
    if [[ -d "${BUILD_DIR}/bin/saves" ]]; then
        mkdir -p "${ROOT_DIR}/saves"
        cp -rn "${BUILD_DIR}/bin/saves/"* "${ROOT_DIR}/saves/" 2>/dev/null || true
    fi
    rm -rf "${BUILD_DIR}"
    echo "Removed ${BUILD_DIR} (Saves preserved in ${ROOT_DIR}/saves)"
else
    echo "Nothing to clean."
fi
