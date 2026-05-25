#pragma once
#include <array>
#include <string>
#include <sodium.h>

namespace meshcompute::core {

struct NodeIdentity {
    std::array<unsigned char, crypto_sign_PUBLICKEYBYTES> public_key;
    std::array<unsigned char, crypto_sign_SECRETKEYBYTES> secret_key;

    static NodeIdentity generate();
    std::string public_key_hex() const;
    std::string sign(const std::string& message) const;
    bool verify(const std::string& message, const std::string& signature) const;
};

} // namespace meshcompute::core