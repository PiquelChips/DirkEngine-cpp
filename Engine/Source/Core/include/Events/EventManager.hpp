#pragma once

#include "Event.hpp"

#include <functional>
#include <memory>
#include <typeindex>

namespace dirk {

struct EventHandle {};

#define DIRK_DISPATCH_EVENT(event, ...) dirk::gEngine->getEventManager()->submitEvent(std::make_unique<event>(__VA_ARGS__));

class EventManager {
public:
    void dispatchEvents();
    void submitEvent(std::unique_ptr<Event> event);

    template <typename T>
    void bindEvent(std::function<bool(T&)> callback) {
        subscribers[typeid(T)].push_back(([callback](Event& event) {
            return callback(static_cast<T&>(event));
        }));
    };

    // TODO: setup EventHandle to be returned with bindEvent that will unbind the event when the EventHandle is destroyed (to avoid dangling pointers)

private:
    using CallbackType = std::function<bool(Event&)>;
    std::vector<std::unique_ptr<Event>> eventQueue;
    std::unordered_map<std::type_index, std::vector<CallbackType>> subscribers;
};

} // namespace dirk
