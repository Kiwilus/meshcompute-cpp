#!/bin/bash
set -e

echo "======================================"
echo " MeshCompute Controller Setup"
echo "======================================"

# Prüfe Abhängigkeiten (nur Hinweise, kein apt-get)
command -v cmake >/dev/null 2>&1 || { echo "cmake wird benötigt, bitte installieren."; exit 1; }
command -v g++ >/dev/null 2>&1 || { echo "g++ wird benötigt, bitte installieren."; exit 1; }

read -p "Server URL (wss://<ip>:<port>): " SERVER_URL
read -sp "Admin Token (aus dem Server-Setup): " ADMIN_TOKEN
echo

# config.json für Controller anpassen
mkdir -p ../config
cat > ../config/config.json <<EOF
{
    "controller": {
        "server_url": "${SERVER_URL}",
        "auth_token": "${ADMIN_TOKEN}"
    }
}
EOF

echo "Baue Controller..."
cd ..
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc) meshcompute-controller

echo ""
echo "======================================"
echo " Controller erfolgreich gebaut."
echo " Starte mit:"
echo "   ./build/controller/meshcompute-controller config/config.json"
echo "======================================"