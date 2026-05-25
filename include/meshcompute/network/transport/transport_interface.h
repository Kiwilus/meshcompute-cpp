#pragma once
#include <string>
#include <functional>
#include <vector>

namespace meshcompute::network::transport {

class TransportInterface {
public:
    virtual ~TransportInterface() = default;
    virtual void send(const std::string& peer_id, const std::vector<uint8_t>& data) = 0;
    using RecvCallback = std::function<void(const std::string& peer_id, const std::vector<uint8_t>&)>;
    virtual void on_receive(RecvCallback) = 0;
};

} // namespace meshcompute::network::transport