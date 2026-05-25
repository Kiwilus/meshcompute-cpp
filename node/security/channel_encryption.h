#pragma once
#include <vector>
#include <string>

namespace meshcompute {

class ChannelEncryption {
public:
    // XChaCha20-Poly1305 encrypted channel
    ChannelEncryption();
    
    // Generate ephemeral keypair for ECDH key exchange
    std::vector<uint8_t> get_public_key() const;
    
    // Derive shared secret from peer's public key
    void derive_shared_secret(const std::vector<uint8_t>& peer_public_key);
    
    // Encrypt/Decrypt
    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext);
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& ciphertext);

private:
    std::vector<uint8_t> shared_secret_;
    // Sodium/Libsodium integration
};

} // namespace meshcompute