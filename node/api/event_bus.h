#pragma once
#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>
#include "common/message.h"

namespace meshcompute {

class EventBus {
public:
    using Callback = std::function<void(const Message&)>;
    
    void subscribe(MessageType type, Callback cb);
    void unsubscribe(MessageType type, Callback cb);
    void publish(const Message& msg);
    
    static EventBus& instance();

private:
    std::mutex mutex_;
    std::unordered_map<MessageType, std::vector<Callback>> subscribers_;
};

} // namespace meshcompute