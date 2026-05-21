add_executable(meshcompute-controller
    main_controller.cpp
    mesh_controller.cpp
)

target_link_libraries(meshcompute-controller
    common
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)