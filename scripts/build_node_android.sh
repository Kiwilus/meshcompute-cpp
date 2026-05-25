#!/bin/bash
# Native Shared Library (.so) für Android bauen, die dann per APK eingebunden wird
set -e

export ANDROID_NDK_HOME=${ANDROID_NDK_HOME:-$HOME/Android/Sdk/ndk/25.2.9519653}
TOOLCHAIN=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake

BUILD_DIR="build/android"
mkdir -p $BUILD_DIR
cmake -B $BUILD_DIR \
    -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-26 \
    -DBUILD_NODE=ON -DBUILD_LEGACY=OFF
cmake --build $BUILD_DIR -- -j4

echo "libmeshcompute.so und mesh-node binary erstellt unter $BUILD_DIR"