file(GLOB NODE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/compute/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/discovery/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/transport/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/security/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/monitoring/*.cpp
)

add_library(mesh_node_lib ${NODE_SOURCES})
target_include_directories(mesh_node_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${SODIUM_INCLUDE}
)
target_link_libraries(mesh_node_lib PUBLIC
    Boost::thread
    pthread
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    ${SODIUM_LIB}
    protobuf::libprotobuf
)

add_executable(mesh-node main_node.cpp)
target_link_libraries(mesh-node PRIVATE mesh_node_lib)
