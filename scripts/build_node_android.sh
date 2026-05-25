#!/bin/bash
set -e

cd "$(dirname "$0")/.."

BUILD_DIR="build-node-android"
echo "Building meshcompute-node for Android (arm64-v8a)..."

# Android NDK Pfad – ggf. anpassen
NDK_PATH="${ANDROID_NDK_HOME:-$HOME/Android/Sdk/ndk/27.0.12077973}"
TOOLCHAIN_FILE="${NDK_PATH}/build/cmake/android.toolchain.cmake"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-24 \
    -DANDROID_STL=c++_shared \
    -DBUILD_SERVER=OFF \
    -DBUILD_CONTROLLER=OFF \
    -DBUILD_BOT=OFF \
    -DBUILD_NODE=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -Dsodium_DIR="${PWD}/../android/app/libs/sodium/lib/cmake/sodium" \
    -DOPENSSL_ROOT_DIR="${PWD}/../android/app/libs/openssl" \
    -DBOOST_ROOT="${PWD}/../android/app/libs/boost"

cmake --build . --target meshcompute-node-exec -j$(nproc)

echo "✅ Node library created at: $(pwd)/node/libmeshcompute-node-exec.so"