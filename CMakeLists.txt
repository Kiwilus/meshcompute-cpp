cmake_minimum_required(VERSION 3.14)
project(meshcompute VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system thread)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

# optional: find_package(redis++ REQUIRED)

add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(controller)
add_subdirectory(bot)