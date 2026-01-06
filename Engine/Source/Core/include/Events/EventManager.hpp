#pragma once

#include "Event.hpp"
#include "logging/logging.hpp"

#include <functional>
#include <memory>
#include <typeindex>

namespace dirk {

#define DIRK_DISPATCH_EVENT(event, ...) dirk::gEngine->getEventManager()->submitEvent(std::make_unique<event>(__VA_ARGS__));

DECLARE_LOG_CATEGORY_EXTERN(LogEvents)

using EventHandle = size_t;
using EventCallback = std::function<bool(Event&)>;

struct EventBinding {
    EventHandle handle;
    EventCallback callback;
};

class EventManager {
public:
    void dispatchEvents();
    void submitEvent(std::unique_ptr<Event> event);

    template <typename T>
    EventHandle bindLambda(std::function<bool(T&)> callback) {
        EventHandle handle = ++lastHandle;
        auto wrapper = [callback](Event& event) {
            return callback(static_cast<T&>(event));
        };

        subscribers[typeid(T)].push_back({ handle, wrapper });
        return handle;
    };

    template <typename T, typename EventType>
    EventHandle bindMember(T* instance, bool (T::*method)(EventType&)) {
        return bindLambda<EventType>([instance, method](EventType& event) {
            return (instance->*method)(event);
        });
    }

    template <typename T>
    void unbind(EventHandle handle) {
        std::type_index type = typeid(T);
        if (subscribers.count(type)) {
            auto& list = subscribers[type];
            list.erase(std::remove_if(list.begin(), list.end(), [handle](const EventBinding& slot) { return slot.handle == handle; }), list.end());
        }
    }

private:
    EventHandle lastHandle;
    std::vector<std::unique_ptr<Event>> eventQueue;
    std::unordered_map<std::type_index, std::vector<EventBinding>> subscribers;
};

} // namespace dirk
