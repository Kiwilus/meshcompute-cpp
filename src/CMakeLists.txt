# Sammle alle Quellen
file(GLOB_RECURSE NODE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

# Erstelle Shared Library
add_library(meshcompute SHARED ${NODE_SOURCES})
target_include_directories(meshcompute PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(meshcompute PUBLIC
    Boost::boost
    OpenSSL::SSL
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    sodium
    protobuf::libprotobuf
)

# Mesh-Node Executable
add_executable(mesh-node main_node.cpp)
target_link_libraries(mesh-node PRIVATE meshcompute)