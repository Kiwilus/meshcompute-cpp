#!/bin/bash
set -e
BUILD_DIR="build/linux"
mkdir -p $BUILD_DIR
cmake -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Release -DBUILD_NODE=ON -DBUILD_LEGACY=ON
cmake --build $BUILD_DIR -- -j$(nproc)
echo "Binaries in $BUILD_DIR/src/mesh-node (und legacy Server/Controller/Bot)"