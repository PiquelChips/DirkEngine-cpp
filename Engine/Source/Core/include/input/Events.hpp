#pragma once

#include "events/Event.hpp"
#include "events/EventManager.hpp"
#include "keys.hpp"

#include "glm/glm.hpp"

namespace dirk {

class KeyboardEvent : public Event {
public:
    KeyboardEvent(Input::Key key, Input::KeyState state) : key(key), state(state) {}

    Input::Key getKey() { return key; }
    Input::KeyState getState() { return state; }

private:
    Input::Key key;
    Input::KeyState state;

    DEFINE_EVENT_TYPE(KeyboardEvent);
};

class MouseButtonEvent : public Event {
public:
    MouseButtonEvent(Input::MouseButton key, Input::KeyState state) : key(key), state(state) {}

    Input::MouseButton getKey() { return key; }
    Input::KeyState getState() { return state; }

private:
    Input::MouseButton key;
    Input::KeyState state;

    DEFINE_EVENT_TYPE(MouseButtonEvent);
};

class MouseMoveEvent : public Event {
public:
    MouseMoveEvent(glm::vec2 position, glm::vec2 offset) : position(position), offset(offset) {}

    // position is global position of surface
    glm::vec2 getPosition() { return position; }
    // offset is the difference between previous position & current position
    glm::vec2 getOffset() { return offset; }

private:
    glm::vec2 position{ .0f, .0f };
    glm::vec2 offset{ .0f, .0f };

    DEFINE_EVENT_TYPE(MouseMoveEvent);
};

} // namespace dirk
