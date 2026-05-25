#!/bin/bash
set -e

# Wechsle ins Projekt-Root (übergeordnetes Verzeichnis des Skript-Verzeichnisses)
cd "$(dirname "$0")/.."

BUILD_DIR="build-node-linux"
echo "Building meshcompute-node for Linux..."
echo "Project root: $(pwd)"

# Abhängigkeiten prüfen
command -v cmake >/dev/null 2>&1 || { echo "cmake not found"; exit 1; }

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. \
    -DBUILD_SERVER=OFF \
    -DBUILD_CONTROLLER=OFF \
    -DBUILD_BOT=OFF \
    -DBUILD_NODE=ON \
    -DCMAKE_BUILD_TYPE=Release

cmake --build . --target meshcompute-node-exec -j$(nproc)

echo "✅ Node binary created at: $(pwd)/node/meshcompute-node-exec"