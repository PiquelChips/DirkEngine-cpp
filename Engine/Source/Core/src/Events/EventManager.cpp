#include "Events/EventManager.hpp"
#include "logging/logging.hpp"

#include <algorithm>
#include <memory>
#include <vector>

namespace dirk {

DEFINE_LOG_CATEGORY(LogEvents)

void EventManager::dispatchEvents() {
    std::vector<std::unique_ptr<Event>> processingQueue = std::move(eventQueue);
    eventQueue = std::vector<std::unique_ptr<Event>>{};

    for (auto& event : processingQueue) {
        auto type = event->getType();
        if (subscribers.count(type)) {
            for (auto& callback : subscribers[type]) {
                callback(*event);
            }
        }
    }
    processingQueue.clear();
}

void EventManager::submitEvent(std::unique_ptr<Event> event) {
    eventQueue.push_back(std::move(event));
}

} // namespace dirk
