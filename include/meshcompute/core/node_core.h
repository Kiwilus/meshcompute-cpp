#pragma once
#include <string>

namespace meshcompute::core {

class NodeCore {
public:
    void initialize(const std::string& config_path);
    void run();
    void shutdown();
};

} // namespace meshcompute::core