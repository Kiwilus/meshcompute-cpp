cmake_minimum_required(VERSION 3.10)
project(meshbot-android)

add_library(bot-core SHARED
    bot_handler.cpp
    jni_bridge.cpp
)


set(BOOST_ROOT ${CMAKE_SOURCE_DIR}/../../../libs/boost)
set(OPENSSL_ROOT ${CMAKE_SOURCE_DIR}/../../../libs/openssl)

target_include_directories(bot-core PRIVATE
    ${BOOST_ROOT}/include
    ${OPENSSL_ROOT}/include
)

target_link_libraries(bot-core
    ${BOOST_ROOT}/lib/${ANDROID_ABI}/libboost_system.a
    ${OPENSSL_ROOT}/lib/${ANDROID_ABI}/libssl.a
    ${OPENSSL_ROOT}/lib/${ANDROID_ABI}/libcrypto.a
    log
)