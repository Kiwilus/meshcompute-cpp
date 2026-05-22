add_library(common STATIC
    config_manager.cpp
    crypto_utils.cpp
)

target_include_directories(common PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(common PUBLIC
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

target_compile_options(common PRIVATE -Wno-deprecated-declarations)