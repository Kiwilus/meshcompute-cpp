#include "node/security/channel_encryption.h"
#include <sodium.h>
#include <stdexcept>

namespace meshcompute {

ChannelEncryption::ChannelEncryption() {
    if (sodium_init() < 0) throw std::runtime_error("libsodium init failed");
}

std::vector<uint8_t> ChannelEncryption::get_public_key() const {
    // Für ECDH über X25519
    std::vector<uint8_t> pk(crypto_scalarmult_BYTES);
    std::vector<uint8_t> sk(crypto_scalarmult_SCALARBYTES);
    // In der Praxis würde man dauerhafte Schlüssel verwenden – hier nur Beispiel
    crypto_box_keypair(pk.data(), sk.data());
    return pk;
}

void ChannelEncryption::derive_shared_secret(const std::vector<uint8_t>& peer_public_key) {
    if (peer_public_key.size() != crypto_scalarmult_BYTES)
        throw std::runtime_error("Invalid public key size");
    // Wir bräuchten den eigenen privaten Schlüssel – stark vereinfacht
    shared_secret_.resize(crypto_scalarmult_BYTES);
    // Dummy: hier müsste crypto_scalarmult aufgerufen werden
}

std::vector<uint8_t> ChannelEncryption::encrypt(const std::vector<uint8_t>& plaintext) {
    // XChaCha20-Poly1305 (vereinfacht)
    std::vector<uint8_t> ciphertext(plaintext.size() + crypto_aead_xchacha20poly1305_ietf_ABYTES);
    unsigned long long clen;
    // Nonce = 0 (in der Praxis zufällig)
    std::vector<uint8_t> nonce(crypto_aead_xchacha20poly1305_ietf_NPUBBYTES, 0);
    crypto_aead_xchacha20poly1305_ietf_encrypt(
        ciphertext.data(), &clen,
        plaintext.data(), plaintext.size(),
        nullptr, 0, nullptr, nonce.data(), shared_secret_.data());
    ciphertext.resize(clen);
    return ciphertext;
}

std::vector<uint8_t> ChannelEncryption::decrypt(const std::vector<uint8_t>& ciphertext) {
    std::vector<uint8_t> plaintext(ciphertext.size() - crypto_aead_xchacha20poly1305_ietf_ABYTES);
    unsigned long long plen;
    std::vector<uint8_t> nonce(crypto_aead_xchacha20poly1305_ietf_NPUBBYTES, 0);
    if (crypto_aead_xchacha20poly1305_ietf_decrypt(
        plaintext.data(), &plen,
        nullptr,
        ciphertext.data(), ciphertext.size(),
        nullptr, 0, nonce.data(), shared_secret_.data()) != 0)
        throw std::runtime_error("Decryption failed");
    plaintext.resize(plen);
    return plaintext;
}

} // namespace meshcompute