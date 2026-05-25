#!/bin/bash
# Installiert alle Abhängigkeiten systemweit
set -e

if [[ "$OSTYPE" == "darwin"* ]]; then
    brew install cmake boost openssl nlohmann-json spdlog libsodium protobuf
else
    sudo apt update
    sudo apt install -y build-essential cmake libboost-all-dev libssl-dev nlohmann-json3-dev libspdlog-dev libsodium-dev protobuf-compiler libprotobuf-dev
fi