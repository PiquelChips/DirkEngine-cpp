#pragma once

#include "Events/Event.hpp"

#include "imgui.h"
#include "input/keys.hpp"
#include "vulkan/vulkan.hpp"

namespace dirk::Platform {

class WindowResizeEvent : public Event {
public:
    DEFINE_EVENT_TYPE(WindowResizeEvent);

    WindowResizeEvent(ImGuiViewport* viewport, vk::Extent2D newSize)
        : viewport(viewport), newSize(newSize) {}

    ImGuiViewport* viewport;
    vk::Extent2D newSize;
};

class WindowMoveEvent : public Event {
public:
    DEFINE_EVENT_TYPE(WindowMoveEvent);

    WindowMoveEvent(ImGuiViewport* viewport) : viewport(viewport) {}

    ImGuiViewport* viewport;
};

class WindowCloseEvent : public Event {
public:
    DEFINE_EVENT_TYPE(WindowCloseEvent);

    WindowCloseEvent(ImGuiViewport* viewport) : viewport(viewport) {}

    ImGuiViewport* viewport;
};

class WindowFocusEvent : public Event {
public:
    DEFINE_EVENT_TYPE(WindowFocusEvent);

    WindowFocusEvent(ImGuiViewport* viewport, bool focused)
        : viewport(viewport), focused(focused) {}

    ImGuiViewport* viewport;
    bool focused;
};

class KeyboardKeyPlatformEvent : public Event {
public:
    DEFINE_EVENT_TYPE(KeyboardKeyPlatformEvent);

    KeyboardKeyPlatformEvent(Input::Key key, Input::KeyState state) : key(key), state(state) {}

    Input::Key key;
    Input::KeyState state;
};

class MouseButtonPlatformEvent : public Event {
public:
    DEFINE_EVENT_TYPE(MouseButtonPlatformEvent);

    MouseButtonPlatformEvent(Input::MouseButton key, Input::KeyState state) : key(key), state(state) {}

    Input::MouseButton key;
    Input::KeyState state;
};

class MouseMovePlatformEvent : public Event {
public:
    DEFINE_EVENT_TYPE(MouseMovePlatformEvent);

    MouseMovePlatformEvent(glm::vec2 position) : position(position) {}

    glm::vec2 position{ .0f, .0f };
};

class MouseScrollPlatformEvent : public Event {
public:
    DEFINE_EVENT_TYPE(MouseScrollPlatformEvent);

    MouseScrollPlatformEvent(glm::vec2 offset) : offset(offset) {}

    glm::vec2 offset{ .0f, .0f };
};

} // namespace dirk::Platform
