#include "node/api/event_bus.h"
#include <algorithm>

namespace meshcompute {

EventBus& EventBus::instance() {
    static EventBus bus;
    return bus;
}

void EventBus::subscribe(MessageType type, Callback cb) {
    std::lock_guard lock(mutex_);
    subscribers_[type].push_back(std::move(cb));
}

void EventBus::unsubscribe(MessageType type, Callback cb) {
    std::lock_guard lock(mutex_);
    auto& vec = subscribers_[type];
    vec.erase(std::remove_if(vec.begin(), vec.end(),
        [&](const Callback& c) { return c.target_type() == cb.target_type(); }),
        vec.end());
}

void EventBus::publish(const Message& msg) {
    std::lock_guard lock(mutex_);
    auto it = subscribers_.find(msg.type);
    if (it != subscribers_.end()) {
        for (auto& cb : it->second) {
            cb(msg);
        }
    }
}

} // namespace meshcompute