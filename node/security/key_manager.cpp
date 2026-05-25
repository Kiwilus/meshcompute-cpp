#include "meshcompute/security/key_manager.h"
#include <sodium.h>
#include <fstream>
#include <stdexcept>
#include <spdlog/spdlog.h>

namespace meshcompute {

KeyManager::KeyManager() {
    keypair_.public_key.resize(crypto_sign_PUBLICKEYBYTES);
    keypair_.private_key.resize(crypto_sign_SECRETKEYBYTES);
}

void KeyManager::generate_keys() {
    crypto_sign_keypair(keypair_.public_key.data(), keypair_.private_key.data());
}

void KeyManager::load_keys(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open key file: " + path);
    in.read(reinterpret_cast<char*>(keypair_.private_key.data()), crypto_sign_SECRETKEYBYTES);
    in.read(reinterpret_cast<char*>(keypair_.public_key.data()), crypto_sign_PUBLICKEYBYTES);
}

void KeyManager::save_keys(const std::string& path) const {
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(keypair_.private_key.data()), crypto_sign_SECRETKEYBYTES);
    out.write(reinterpret_cast<const char*>(keypair_.public_key.data()), crypto_sign_PUBLICKEYBYTES);
}

std::string KeyManager::sign(const std::string& data) const {
    unsigned char sig[crypto_sign_BYTES];
    unsigned long long sig_len;
    crypto_sign_detached(sig, &sig_len,
                         reinterpret_cast<const unsigned char*>(data.data()), data.size(),
                         keypair_.private_key.data());
    return std::string(reinterpret_cast<char*>(sig), sig_len);
}

bool KeyManager::verify(const std::string& data, const std::string& signature,
                        const std::vector<uint8_t>& public_key) const {
    if (public_key.size() != crypto_sign_PUBLICKEYBYTES)
        return false;
    return crypto_sign_verify_detached(
        reinterpret_cast<const unsigned char*>(signature.data()),
        reinterpret_cast<const unsigned char*>(data.data()), data.size(),
        public_key.data()) == 0;
}

std::string KeyManager::get_node_id() const {
    // Node-ID = Base64(SHA256(public_key)).substr(0,16)
    unsigned char hash[crypto_hash_sha256_BYTES];
    crypto_hash_sha256(hash, keypair_.public_key.data(), keypair_.public_key.size());
    // Einfache Hex-Darstellung
    std::stringstream ss;
    for (int i = 0; i < 16; ++i)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return ss.str();
}

} // namespace meshcompute