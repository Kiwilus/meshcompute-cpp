#!/bin/bash
set -e

echo "======================================"
echo " MeshCompute Controller Setup"
echo "======================================"

# Benötigte Pakete sicherstellen
echo "Prüfe Abhängigkeiten..."
if command -v pacman &>/dev/null; then
    sudo pacman -S --needed --noconfirm cmake make boost boost-libs openssl nlohmann-json spdlog
elif command -v apt &>/dev/null; then
    sudo apt update && sudo apt install -y cmake build-essential libboost-all-dev libssl-dev nlohmann-json3-dev libspdlog-dev
fi

# Pfade zu Boost finden
BOOST_INC="/usr/include"
BOOST_SYS_LIB="/usr/lib/libboost_system.so"
BOOST_THR_LIB="/usr/lib/libboost_thread.so"

if [ ! -f "$BOOST_SYS_LIB" ]; then
    # Versuche statische Variante
    BOOST_SYS_LIB="/usr/lib/libboost_system.a"
    BOOST_THR_LIB="/usr/lib/libboost_thread.a"
fi

if [ ! -f "$BOOST_SYS_LIB" ]; then
    echo "Boost system library nicht gefunden. Bitte installiere boost-libs."
    exit 1
fi

echo "Boost system: $BOOST_SYS_LIB"
echo "Boost thread: $BOOST_THR_LIB"

# Controller-Konfiguration abfragen
read -p "Server URL (wss://<ip>:<port>): " SERVER_URL
read -sp "Admin Token (aus dem Server-Setup): " ADMIN_TOKEN
echo

mkdir -p ../config
cat > ../config/config.json <<EOF
{
    "controller": {
        "server_url": "${SERVER_URL}",
        "auth_token": "${ADMIN_TOKEN}"
    }
}
EOF

# Build vorbereiten
echo "Baue Controller..."
cd ..
rm -rf build-controller
mkdir build-controller && cd build-controller

# CMake mit expliziten Pfaden aufrufen
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SERVER=OFF \
    -DBUILD_BOT=OFF \
    -DBoost_INCLUDE_DIR="$BOOST_INC" \
    -DBoost_SYSTEM_LIBRARY_RELEASE="$BOOST_SYS_LIB" \
    -DBoost_THREAD_LIBRARY_RELEASE="$BOOST_THR_LIB" \
    -DBoost_DIR="/usr/lib/cmake/Boost-1.91.0"   # ggf. anpassen

make -j$(nproc) meshcompute-controller

echo ""
echo "======================================"
echo " Controller erfolgreich gebaut."
echo " Starte mit:"
echo "   ./build-controller/controller/meshcompute-controller config/config.json"
echo "======================================"