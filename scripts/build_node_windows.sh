#!/bin/bash
# Cross-Compile für Windows mit MinGW-w64 (auf Linux ausgeführt)
set -e
sudo apt install -y mingw-w64

TOOLCHAIN=$(pwd)/scripts/toolchain-mingw.cmake
cat > $TOOLCHAIN << 'EOF'
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
EOF

BUILD_DIR="build/windows"
mkdir -p $BUILD_DIR
cmake -B $BUILD_DIR \
    -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_NODE=ON -DBUILD_LEGACY=OFF
cmake --build $BUILD_DIR -- -j$(nproc)
echo "Windows .exe unter $BUILD_DIR/src/mesh-node.exe"