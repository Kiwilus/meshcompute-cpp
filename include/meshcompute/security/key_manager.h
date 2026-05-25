#pragma once
#include <string>
#include <array>
#include <sodium.h>

namespace meshcompute::security {

class KeyManager {
public:
    void generate();
    std::string public_key_hex() const;
    std::array<unsigned char, crypto_sign_SECRETKEYBYTES> secret_key;
    std::array<unsigned char, crypto_sign_PUBLICKEYBYTES> public_key;
};

} // namespace meshcompute::security