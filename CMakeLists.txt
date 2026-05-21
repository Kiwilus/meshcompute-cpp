cmake_minimum_required(VERSION 3.14)
project(meshcompute VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_SERVER "Build server" ON)
option(BUILD_CONTROLLER "Build controller" ON)
option(BUILD_BOT "Build bot" ON)

find_package(Boost CONFIG REQUIRED COMPONENTS system thread)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

add_subdirectory(common)

if(BUILD_SERVER)
    add_subdirectory(server)
endif()
if(BUILD_CONTROLLER)
    add_subdirectory(controller)
endif()
if(BUILD_BOT)
    add_subdirectory(bot)
endif()