#!/bin/bash
set -e

cd "$(dirname "$0")/.."

BUILD_DIR="build-node-windows"
echo "Building meshcompute-node for Windows (MinGW)..."

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../windows/toolchain-mingw.cmake \
    -DBUILD_SERVER=OFF \
    -DBUILD_CONTROLLER=OFF \
    -DBUILD_BOT=OFF \
    -DBUILD_NODE=ON \
    -DCMAKE_BUILD_TYPE=Release

cmake --build . --target meshcompute-node-exec -j$(nproc)

echo "✅ Node binary created at: $(pwd)/node/meshcompute-node-exec.exe"