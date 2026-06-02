#!/bin/bash

set -euo pipefail

CLEAN="${1:-}"
BUILD_DIR="build"
GENERATOR="Ninja"

if [ "$CLEAN" = "--clean" ]; then
    echo "Cleaning build directory..."
    rm -rf "${BUILD_DIR}"
fi

mkdir -p "${BUILD_DIR}"

CACHE_FILE="${BUILD_DIR}/CMakeCache.txt"
if [ -f "${CACHE_FILE}" ]; then
    CACHED_GENERATOR=$(sed -n 's/^CMAKE_GENERATOR:INTERNAL=//p' "${CACHE_FILE}")
    CACHED_QT_DIR=$(sed -n 's/^Qt6_DIR:PATH=//p' "${CACHE_FILE}")

    if [ "${CACHED_GENERATOR}" != "${GENERATOR}" ] || [[ "${CACHED_QT_DIR}" == /opt/qt/* ]]; then
        echo "Reconfiguring build directory for current toolchain..."
        rm -rf "${BUILD_DIR}/CMakeCache.txt" "${BUILD_DIR}/CMakeFiles"
    fi
fi

cd "${BUILD_DIR}"

if [ -n "${CMAKE_PREFIX_PATH:-}" ]; then
    cmake .. -G "${GENERATOR}" -DCMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH}"
else
    cmake .. -G "${GENERATOR}"
fi

cmake --build .
