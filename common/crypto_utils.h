#pragma once
#include <string>

std::string sha256(const std::string& data);
bool verify_hash(const std::string& data, const std::string& hash);