#include "crypto_miner_worker.h"
#include <cstring>
#include <openssl/sha.h>
#include <nlohmann/json.hpp>
#include <random>
#include <sstream>
#include <iomanip>

namespace meshcompute {

nlohmann::json CryptoMinerWorker::execute(const Job& job) {
    // Einfacher Proof-of-Work: Finde Nonce, sodass SHA256(challenge + nonce) mit 4 Nullen beginnt
    std::string challenge = job.parameters.value("challenge", "meshcompute");
    int difficulty = job.parameters.value("difficulty", 4); // führende Nullen im Hash (in Bytes)
    int max_iterations = job.parameters.value("max_iterations", 1000000);

    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;

    for (int i = 0; i < max_iterations; ++i) {
        uint64_t nonce = dis(gen);
        std::string data = challenge + std::to_string(nonce);
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), hash);
        
        // Prüfe führende Null-Bytes
        bool valid = true;
        for (int b = 0; b < difficulty; ++b) {
            if (hash[b] != 0) {
                valid = false;
                break;
            }
        }
        if (valid) {
            std::stringstream ss;
            for (int j = 0; j < SHA256_DIGEST_LENGTH; ++j)
                ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[j];
            nlohmann::json result;
            result["nonce"] = nonce;
            result["hash"] = ss.str();
            result["iterations"] = i + 1;
            return result;
        }
    }
    throw std::runtime_error("No valid nonce found within iteration limit");
}

} // namespace meshcompute