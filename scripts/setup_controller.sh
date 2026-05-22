#!/bin/bash
set -e

echo "======================================"
echo " MeshCompute Controller Setup (.env)"
echo "======================================"

# --- Eingaben ---
read -p "Server IP / Domain (z.B. 192.168.1.188): " SERVER_IP
read -p "Server Port [8443]: " SERVER_PORT
SERVER_PORT=${SERVER_PORT:-8443}

read -p "Admin Token (Klartext vom Server): " ADMIN_TOKEN

if [ -z "$ADMIN_TOKEN" ]; then
    echo "❌ Fehler: Admin Token wird benötigt!"
    exit 1
fi

# --- .env Datei erstellen ---
cat > ../controller/.env << EOF
# MeshCompute Controller Configuration
SERVER_URL=wss://${SERVER_IP}:${SERVER_PORT}
AUTH_TOKEN=${ADMIN_TOKEN}
EOF

echo ""
echo "✅ .env Datei erfolgreich erstellt/aktualisiert!"
echo "======================================"
echo "SERVER_URL  = wss://${SERVER_IP}:${SERVER_PORT}"
echo "AUTH_TOKEN  = ${ADMIN_TOKEN}"
echo ""
echo "Nächste Schritte:"
echo "   cd ../controller"
echo "   python main.py"
echo "======================================"