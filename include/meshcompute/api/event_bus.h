#pragma once
#include <functional>
#include <string>
#include <vector>
#include <any>

namespace meshcompute::api {

class EventBus {
public:
    using Callback = std::function<void(const std::any&)>;
    void subscribe(const std::string& event, Callback cb);
    void publish(const std::string& event, const std::any& data);
};

} // namespace meshcompute::api