cmake_minimum_required(VERSION 3.20)
project(meshcompute VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_NODE "Mesh-Node" ON)

find_package(Boost REQUIRED COMPONENTS thread)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)
find_package(Protobuf REQUIRED)

# sodium manuell finden
find_library(SODIUM_LIB sodium)
find_path(SODIUM_INCLUDE sodium.h)
if(NOT SODIUM_LIB)
    message(FATAL_ERROR "libsodium not found!")
endif()

if(BUILD_NODE)
    add_subdirectory(node)
endif()
