#!/bin/bash
set -e

echo "======================================"
echo " Windows Bot Build (EXE)"
echo "======================================"

command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1 || {
    echo "mingw-w64 wird benötigt. Installiere mit:"
    echo "  sudo apt install mingw-w64"
    exit 1
}

read -p "Server URL (wss://<ip>:<port>): " SERVER_URL
read -sp "Registration Token: " REG_TOKEN
echo
read -p "Bot ID (optional): " BOT_ID

# config.json für Bot anpassen
mkdir -p ../config
cat > ../config/config.json <<EOF
{
    "bot": {
        "server_url": "${SERVER_URL}",
        "registration_token": "${REG_TOKEN}",
        "bot_id": "${BOT_ID}"
    }
}
EOF

# Windows-Build mit statischer Linker-Flags
cd ..
rm -rf build-win
mkdir build-win && cd build-win
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../windows/toolchain-mingw.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXE_LINKER_FLAGS="-static -static-libgcc -static-libstdc++" \
    -DBoost_USE_STATIC_LIBS=ON
make -j$(nproc) meshcompute-bot

echo ""
echo "======================================"
echo " Windows Bot erstellt: build-win/bot/meshcompute-bot.exe"
echo "======================================"