#!/bin/bash
set -e

echo "======================================"
echo " MeshCompute Windows Bot Builder"
echo "======================================"

# --- 1. Benötigte Tools auf dem Host (Linux) ---
command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1 || {
    echo "❌ mingw-w64 nicht gefunden. Installiere mit:"
    echo "   sudo pacman -S mingw-w64"
    exit 1
}

command -v wget >/dev/null 2>&1 || { echo "❌ wget benötigt. sudo pacman -S wget"; exit 1; }
command -v tar >/dev/null 2>&1 || { echo "❌ tar benötigt."; exit 1; }

# Prüfen ob tar zstd unterstützt
if ! tar --help | grep -q zstd; then
    echo "❌ tar ohne zstd-Unterstützung. Installiere zstd: sudo pacman -S zstd"
    exit 1
fi

MINGW_PREFIX="/usr/x86_64-w64-mingw32"
MINGW_INC="$MINGW_PREFIX/include"
MINGW_LIB="$MINGW_PREFIX/lib"

# --- 2. MinGW-w64 Boost automatisch bereitstellen ---
BOOST_URL="https://repo.msys2.org/mingw/mingw64/mingw-w64-x86_64-boost-1.83.0-1-any.pkg.tar.zst"
BOOST_PKG="/tmp/boost-mingw.pkg.tar.zst"

if [ ! -f "$MINGW_INC/boost/version.hpp" ]; then
    echo "⬇️  Lade Boost für mingw-w64 herunter (ca. 50 MB)..."
    wget --show-progress -O "$BOOST_PKG" "$BOOST_URL" || {
        echo "❌ Download fehlgeschlagen. Prüfe die Internetverbindung oder lade manuell:"
        echo "   $BOOST_URL"
        exit 1
    }
    echo "📦 Entpacke Boost nach $MINGW_PREFIX..."
    sudo tar --zstd -xf "$BOOST_PKG" -C / --strip-components=1 2>/dev/null || {
        echo "❌ Entpacken fehlgeschlagen. Paket beschädigt oder nicht genug Platz?"
        exit 1
    }
    rm "$BOOST_PKG"
    echo "✅ Boost installiert."
fi

# OpenSSL
OPENSSL_URL="https://repo.msys2.org/mingw/mingw64/mingw-w64-x86_64-openssl-3.1.1-1-any.pkg.tar.zst"
OPENSSL_PKG="/tmp/openssl-mingw.pkg.tar.zst"

if [ ! -f "$MINGW_INC/openssl/ssl.h" ]; then
    echo "⬇️  Lade OpenSSL für mingw-w64 herunter (ca. 10 MB)..."
    wget --show-progress -O "$OPENSSL_PKG" "$OPENSSL_URL" || {
        echo "❌ Download fehlgeschlagen."
        exit 1
    }
    echo "📦 Entpacke OpenSSL..."
    sudo tar --zstd -xf "$OPENSSL_PKG" -C / --strip-components=1 2>/dev/null || {
        echo "❌ Entpacken fehlgeschlagen."
        exit 1
    }
    rm "$OPENSSL_PKG"
    echo "✅ OpenSSL installiert."
fi

# --- 3. Server-Daten abfragen ---
read -p "Server URL (wss://<ip>:<port>): " SERVER_URL
read -sp "Registration Token: " REG_TOKEN
echo

# --- 4. Eindeutige Bot-ID generieren ---
BOT_ID="bot-$(hostname)-$(date +%m%d%H%M%S)"
echo "🤖 Bot-ID: $BOT_ID"

# --- 5. config.json für Bot erstellen ---
mkdir -p ../config
cat > ../config/config_bot.json <<EOF
{
    "bot": {
        "server_url": "${SERVER_URL}",
        "registration_token": "${REG_TOKEN}",
        "bot_id": "${BOT_ID}"
    }
}
EOF

# --- 6. Projektverzeichnis & Build ---
cd ..
rm -rf build-windows
mkdir build-windows && cd build-windows

# Temporäre CMakeLists.txt (wie vorher)
cat > CMakeLists.txt <<'EOF'
cmake_minimum_required(VERSION 3.14)
project(meshcompute-bot-windows LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

find_path(BOOST_INCLUDE_DIR boost/version.hpp
    PATHS /usr/x86_64-w64-mingw32/include)
find_library(BOOST_SYSTEM_LIBRARY boost_system
    PATHS /usr/x86_64-w64-mingw32/lib)
find_library(BOOST_THREAD_LIBRARY boost_thread
    PATHS /usr/x86_64-w64-mingw32/lib)

if(NOT BOOST_INCLUDE_DIR OR NOT BOOST_SYSTEM_LIBRARY OR NOT BOOST_THREAD_LIBRARY)
    message(FATAL_ERROR "Windows Boost nicht gefunden.")
endif()

add_library(Boost::system UNKNOWN IMPORTED)
set_target_properties(Boost::system PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${BOOST_INCLUDE_DIR}"
    IMPORTED_LOCATION "${BOOST_SYSTEM_LIBRARY}")

add_library(Boost::thread UNKNOWN IMPORTED)
set_target_properties(Boost::thread PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${BOOST_INCLUDE_DIR}"
    IMPORTED_LOCATION "${BOOST_THREAD_LIBRARY}")

find_package(Threads REQUIRED)
set_target_properties(Boost::thread PROPERTIES INTERFACE_LINK_LIBRARIES Threads::Threads)

find_path(OPENSSL_INCLUDE_DIR openssl/ssl.h
    PATHS /usr/x86_64-w64-mingw32/include)
find_library(OPENSSL_SSL_LIBRARY libssl.a
    PATHS /usr/x86_64-w64-mingw32/lib)
find_library(OPENSSL_CRYPTO_LIBRARY libcrypto.a
    PATHS /usr/x86_64-w64-mingw32/lib)

if(NOT OPENSSL_INCLUDE_DIR OR NOT OPENSSL_SSL_LIBRARY OR NOT OPENSSL_CRYPTO_LIBRARY)
    message(FATAL_ERROR "Windows OpenSSL nicht gefunden.")
endif()

add_library(OpenSSL::SSL UNKNOWN IMPORTED)
set_target_properties(OpenSSL::SSL PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}"
    IMPORTED_LOCATION "${OPENSSL_SSL_LIBRARY}"
    INTERFACE_LINK_LIBRARIES "${OPENSSL_CRYPTO_LIBRARY}")

add_library(OpenSSL::Crypto UNKNOWN IMPORTED)
set_target_properties(OpenSSL::Crypto PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}"
    IMPORTED_LOCATION "${OPENSSL_CRYPTO_LIBRARY}")

find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/..)

add_subdirectory(${CMAKE_SOURCE_DIR}/../common common_build)
add_subdirectory(${CMAKE_SOURCE_DIR}/../bot bot_build)
EOF

cmake . \
    -DCMAKE_TOOLCHAIN_FILE=../windows/toolchain-mingw.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXE_LINKER_FLAGS="-static -static-libgcc -static-libstdc++"

make -j$(nproc) meshcompute-bot

echo ""
echo "======================================"
echo " ✅ Windows Bot erstellt:"
echo "    build-windows/bot_build/meshcompute-bot.exe"
echo "    Konfiguration: config/config_bot.json"
echo "======================================"