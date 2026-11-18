#!/usr/bin/env bash
# NEXO Engine - build de un comando para Linux/WSL.
# Requisitos: CMake >= 3.24, ninja y las librerias dev de SFML 3
# (libgl1-mesa-dev libx11-dev libxrandr-dev libxcursor-dev libxi-dev
#  libudev-dev libfreetype-dev en Debian/Ubuntu).
# Uso:  ./build.sh [--test] [--run]
set -euo pipefail

RUN=false
TEST=false
for arg in "$@"; do
    case "$arg" in
        --test) TEST=true ;;
        --run)  RUN=true ;;
        *) echo "Uso: $0 [--test] [--run]"; exit 1 ;;
    esac
done

BUILD_DIR="build-linux"

echo "==> Configurando (Ninja, Release)"
cmake -S . -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release

echo "==> Compilando"
cmake --build "$BUILD_DIR" -j "$(nproc)"

if [ "$TEST" = true ]; then
    echo "==> Corriendo tests"
    ctest --test-dir "$BUILD_DIR" --output-on-failure
fi

if [ "$RUN" = true ]; then
    echo "==> Lanzando NexoEngine"
    cd "$BUILD_DIR" && ./NexoEngine
else
    echo "Listo: $BUILD_DIR/NexoEngine"
fi
