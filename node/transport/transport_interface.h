#pragma once
#include <string>
#include <functional>
#include <vector>

namespace meshcompute {

class ITransport {
public:
    virtual ~ITransport() = default;
    
    virtual bool connect(const std::string& address, uint16_t port) = 0;
    virtual void disconnect() = 0;
    virtual bool is_connected() const = 0;
    
    virtual void send(const std::vector<uint8_t>& data) = 0;
    
    using ReceiveCallback = std::function<void(const std::vector<uint8_t>&)>;
    virtual void set_receive_callback(ReceiveCallback cb) = 0;
    
    virtual std::string transport_type() const = 0;
};

} // namespace meshcompute