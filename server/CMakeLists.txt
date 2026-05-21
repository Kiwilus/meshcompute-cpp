add_executable(meshcompute-server
    main_server.cpp
    hub.cpp
    client_handler.cpp
)

target_include_directories(meshcompute-server PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(meshcompute-server
    common
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)