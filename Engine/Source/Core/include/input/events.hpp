#pragma once

#include "Events/Event.hpp"
#include "keys.hpp"

#include "glm/glm.hpp"
#include <format>

namespace dirk::Input {

class KeyboardKeyEvent : public Event {
public:
    DEFINE_EVENT_TYPE(KeyboardKeyEvent);

    KeyboardKeyEvent(Input::Key key, Input::KeyState state) : key(key), state(state) {}

    Input::Key key;
    Input::KeyState state;
};

class MouseButtonEvent : public Event {
public:
    DEFINE_EVENT_TYPE(MouseButtonEvent);

    MouseButtonEvent(Input::MouseButton button, Input::KeyState state) : button(button), state(state) {}

    Input::MouseButton button;
    Input::KeyState state;
};

// TODO: overhaul input routing to viewports
/*
class MouseMoveEvent : public Event {
public:
    DEFINE_EVENT_TYPE(MouseMoveEvent);

    MouseMoveEvent(glm::vec2 position, glm::vec2 offset) : position(position), offset(offset) {}

    // position is global position of surface
    glm::vec2 position{ .0f, .0f };
    // offset is the difference between previous position & current position
    glm::vec2 offset{ .0f, .0f };
};
*/

class MouseScrollEvent : public Event {
public:
    DEFINE_EVENT_TYPE(MouseScrollEvent);

    MouseScrollEvent(glm::vec2 offset) : offset(offset) {}

    glm::vec2 offset{ .0f, .0f };
};

} // namespace dirk::Input
