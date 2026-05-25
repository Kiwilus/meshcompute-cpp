cmake_minimum_required(VERSION 3.14)

# Collect source files
file(GLOB_RECURSE NODE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(meshcompute-node STATIC ${NODE_SOURCES})

target_include_directories(meshcompute-node
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(meshcompute-node
    PUBLIC
        meshcompute-common
        Boost::system
        Boost::thread
        OpenSSL::SSL
        OpenSSL::Crypto
        sodium
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)

# Main executable for a standalone mesh node
add_executable(meshcompute-node-exec main_node.cpp)
target_link_libraries(meshcompute-node-exec PRIVATE meshcompute-node)

# Copy example config if building
configure_file(
    ${CMAKE_SOURCE_DIR}/config/mesh_node.json
    ${CMAKE_BINARY_DIR}/mesh_node.json
    COPYONLY
)