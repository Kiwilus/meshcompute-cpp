#!/bin/bash
set -e

echo "======================================"
echo " MeshCompute Server Deployment Setup"
echo "======================================"

# --- Konfiguration abfragen ---
read -p "VPS IP / Domain (z.B. 192.168.1.188): " VPS_IP
read -p "Server Port [8443]: " SERVER_PORT
SERVER_PORT=${SERVER_PORT:-8443}

# --- Secrets generieren ---
echo "Generiere Secrets..."
ADMIN_TOKEN=$(openssl rand -hex 32)
REG_TOKEN=$(openssl rand -hex 32)
ADMIN_HASH=$(echo -n "$ADMIN_TOKEN" | openssl dgst -sha256 | awk '{print $2}')
REG_HASH=$(echo -n "$REG_TOKEN" | openssl dgst -sha256 | awk '{print $2}')

# --- TLS-Zertifikate sauber erzeugen ---
echo "Generiere TLS-Zertifikate..."
mkdir -p certs
rm -f certs/server.crt certs/server.key

openssl req -x509 -newkey rsa:2048 -keyout certs/server.key -out certs/server.crt \
    -days 365 -nodes -subj "/CN=${VPS_IP}" 2>/dev/null

chmod 600 certs/server.key
chmod 644 certs/server.crt

echo "Zertifikate erfolgreich erstellt."

# --- Ordnerstruktur für Deployment ---
rm -rf deploy
mkdir -p deploy/meshcompute-cpp/{config,certs,docker}

# --- Nur die wirklich nötigen Dateien kopieren (Server-only) ---
echo "Kopiere minimale Server-Dateien..."

cp -r ../server/                  deploy/meshcompute-cpp/server/
cp -r ../common/                  deploy/meshcompute-cpp/common/
cp ../CMakeLists.txt              deploy/meshcompute-cpp/
cp ../docker/Dockerfile.server    deploy/meshcompute-cpp/docker/Dockerfile.server 2>/dev/null || true

# Falls du noch weitere benötigte Ordner/Dateien hast, füge sie hier hinzu:
# cp -r ../config/                deploy/meshcompute-cpp/config/   # falls nötig

# --- config.json ---
cat > deploy/meshcompute-cpp/config/config.json <<EOF
{
    "server": {
        "host": "0.0.0.0",
        "port": ${SERVER_PORT},
        "tls_cert_file": "/etc/meshcompute/certs/server.crt",
        "tls_key_file": "/etc/meshcompute/certs/server.key",
        "admin_token_hash": "${ADMIN_HASH}",
        "registration_token_hash": "${REG_HASH}"
    }
}
EOF

# --- Zertifikate kopieren ---
cp certs/server.crt certs/server.key deploy/meshcompute-cpp/certs/

# --- docker-compose.yml ---
cat > deploy/docker-compose.yml <<EOF
version: '3'
services:
  server:
    build:
      context: ./meshcompute-cpp
      dockerfile: docker/Dockerfile.server
    ports:
      - "${SERVER_PORT}:${SERVER_PORT}"
    volumes:
      - ./meshcompute-cpp/config/config.json:/etc/meshcompute/config.json
      - ./meshcompute-cpp/certs:/etc/meshcompute/certs:ro
    restart: unless-stopped
EOF

echo ""
echo "======================================"
echo " Deployment-Paket erfolgreich erstellt!"
echo "======================================"
echo "Admin Token: $ADMIN_TOKEN"
echo "Registration Token: $REG_TOKEN"
echo ""
echo "Nächste Schritte:"
echo " 1. scp -r deploy/ kiwi@${VPS_IP}:~/meshcompute"
echo " 2. ssh kiwi@${VPS_IP}"
echo " 3. cd ~/meshcompute && sudo docker compose down --rmi all"
echo " 4. sudo docker compose up -d --build"
echo "======================================"