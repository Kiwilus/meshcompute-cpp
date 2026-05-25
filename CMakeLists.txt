cmake_minimum_required(VERSION 3.20)
project(meshcompute VERSION 2.0.0 LANGUAGES CXX)

# C++17 ist Pflicht
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optionen für modulare Builds
option(BUILD_LEGACY "Baue alte Server/Controller/Bot-Komponenten" ON)
option(BUILD_NODE "Baue neuen dezentralen Mesh-Node" ON)
option(USE_MDNS "Aktiviere mDNS-Discovery" ON)
option(USE_LIBP2P "Aktiviere libp2p-Transport (experimentell)" OFF)
option(USE_GRPC "Aktiviere gRPC-API" OFF)
option(BUILD_TESTS "Tests bauen" OFF)

# Abhängigkeiten suchen
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

# libsodium für moderne Kryptografie
find_package(sodium REQUIRED)

# Protobuf (für gRPC, immer benötigt für Nachrichten)
find_package(Protobuf REQUIRED)

# legacy Quellen einbinden (optional)
if(BUILD_LEGACY)
    add_subdirectory(server)
    add_subdirectory(controller)
    add_subdirectory(bot)
endif()

# Neue Node-Bibliothek und Executable
if(BUILD_NODE)
    add_subdirectory(src)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()