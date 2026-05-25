#pragma once
#include <string>
#include <vector>

namespace meshcompute::security {

class ChannelEncryption {
public:
    std::string encrypt(const std::string& plain, const std::string& peer_pubkey);
    std::string decrypt(const std::string& cipher, const std::string& peer_pubkey);
};

} // namespace meshcompute::security