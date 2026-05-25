#pragma once
#include <string>
#include <vector>

namespace meshcompute {

struct KeyPair {
    std::vector<uint8_t> public_key;   // Ed25519: 32 bytes
    std::vector<uint8_t> private_key;  // Ed25519: 32 bytes
};

class KeyManager {
public:
    KeyManager();
    
    void generate_keys();
    void load_keys(const std::string& path);
    void save_keys(const std::string& path) const;
    
    std::string sign(const std::string& data) const;
    bool verify(const std::string& data, const std::string& signature, 
                const std::vector<uint8_t>& public_key) const;
    
    const KeyPair& get_keypair() const { return keypair_; }
    std::string get_node_id() const;  // Derived from public key hash

private:
    KeyPair keypair_;
};

} // namespace meshcompute