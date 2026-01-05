#include "events/EventManager.hpp"

#include <algorithm>
#include <memory>

namespace dirk {

void EventManager::dispatchEvents() {
    for (auto& event : eventQueue) {
        auto type = event->getType();
        if (subscribers.count(type)) {
            for (auto& callback : subscribers[type]) {
                callback(*event);
            }
        }
    }
    eventQueue.clear();
}

void EventManager::submitEvent(std::unique_ptr<Event> event) {
    eventQueue.push_back(std::move(event));
}

} // namespace dirk
