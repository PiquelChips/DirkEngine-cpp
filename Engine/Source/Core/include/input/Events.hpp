#pragma once

#include "events/Event.hpp"
#include "events/EventManager.hpp"
#include "keys.hpp"

#include "glm/glm.hpp"

namespace dirk {

class KeyboardEvent : public Event {
public:
    DEFINE_EVENT_TYPE(KeyboardEvent);

    KeyboardEvent(Input::Key key, Input::KeyState state) : key(key), state(state) {}

    Input::Key key;
    Input::KeyState state;
};

class MouseButtonEvent : public Event {
public:
    DEFINE_EVENT_TYPE(MouseButtonEvent);

    MouseButtonEvent(Input::MouseButton key, Input::KeyState state) : key(key), state(state) {}

    Input::MouseButton key;
    Input::KeyState state;
};

class MouseMoveEvent : public Event {
public:
    DEFINE_EVENT_TYPE(MouseMoveEvent);

    MouseMoveEvent(glm::vec2 position, glm::vec2 offset) : position(position), offset(offset) {}

    // position is global position of surface
    glm::vec2 position{ .0f, .0f };
    // offset is the difference between previous position & current position
    glm::vec2 offset{ .0f, .0f };
};

} // namespace dirk
