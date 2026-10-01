#!/usr/bin/env bash
# Runs the built game from its output directory (so assets resolve).
# Usage:  ./scripts/run.sh

set -euo pipefail

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

cd "${BIN_DIR}"
exec "${EXE}" "$@"
