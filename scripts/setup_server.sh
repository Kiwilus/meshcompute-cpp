#!/bin/bash
set -e

echo "======================================"
echo " MeshCompute Server Deployment Setup"
echo "======================================"

# --- Konfiguration abfragen ---
read -p "VPS IP / Domain (z.B. 12.34.56.78): " VPS_IP
read -p "Server Port [8443]: " SERVER_PORT
SERVER_PORT=${SERVER_PORT:-8443}

# --- Secrets generieren ---
echo "Generiere Secrets..."
ADMIN_TOKEN=$(openssl rand -hex 32)
REG_TOKEN=$(openssl rand -hex 32)
ADMIN_HASH=$(echo -n "$ADMIN_TOKEN" | openssl dgst -sha256 | awk '{print $2}')
REG_HASH=$(echo -n "$REG_TOKEN" | openssl dgst -sha256 | awk '{print $2}')

# --- TLS-Zertifikate erzeugen ---
mkdir -p certs
openssl req -x509 -newkey rsa:4096 -keyout certs/server.key -out certs/server.crt \
    -days 365 -nodes -subj "/CN=${VPS_IP}" 2>/dev/null

# --- Ordnerstruktur für Deployment erstellen ---
rm -rf deploy
mkdir -p deploy/meshcompute-cpp/{config,certs}
mkdir -p deploy/meshcompute-cpp/docker

# --- Projektdateien kopieren (nur das Nötigste) ---
# Wir kopieren den gesamten Quellcode, CMakeLists.txt und Dockerfiles
echo "Kopiere Projektdateien..."
rsync -a --exclude='deploy' --exclude='.git' --exclude='build' --exclude='android' \
    --exclude='windows' --exclude='*.apk' --exclude='*.exe' \
    ../ deploy/meshcompute-cpp/

# config.json für den Server anpassen
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

# Zertifikate in den deploy-Ordner legen
cp certs/server.crt certs/server.key deploy/meshcompute-cpp/certs/

# Dockerfile.server (unverändert) mitkopieren – wir lassen es, wie es ist
# docker-compose.yml für den Server erstellen
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
      - ./meshcompute-cpp/certs:/etc/meshcompute/certs
    restart: unless-stopped
EOF

echo ""
echo "======================================"
echo " Deployment-Paket erstellt unter 'deploy/'"
echo "======================================"
echo "WICHTIG: Bewahre diese Tokens sicher auf!"
echo " Admin Token: $ADMIN_TOKEN"
echo " Registration Token: $REG_TOKEN"
echo ""
echo "Nächste Schritte:"
echo " 1. scp -r deploy/ user@${VPS_IP}:~/meshcompute"
echo " 2. ssh user@${VPS_IP}"
echo " 3. cd ~/meshcompute && docker compose up -d --build"
echo "======================================"