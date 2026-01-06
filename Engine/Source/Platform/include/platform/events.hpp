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

    KeyboardKeyPlatformEvent(ImGuiViewport* viewport, Input::Key key, Input::KeyState state)
        : viewport(viewport), key(key), state(state) {}

    ImGuiViewport* viewport;
    Input::Key key;
    Input::KeyState state;
};

class KeyboardCharPlatformEvent : public Event {
public:
    DEFINE_EVENT_TYPE(KeyboardCharPlatformEvent);

    KeyboardCharPlatformEvent(ImGuiViewport* viewport, unsigned int c)
        : viewport(viewport), c(c) {}

    ImGuiViewport* viewport;
    unsigned int c;
};

class MouseButtonPlatformEvent : public Event {
public:
    DEFINE_EVENT_TYPE(MouseButtonPlatformEvent);

    MouseButtonPlatformEvent(ImGuiViewport* viewport, Input::MouseButton button, Input::KeyState state)
        : viewport(viewport), button(button), state(state) {}

    ImGuiViewport* viewport;
    Input::MouseButton button;
    Input::KeyState state;
};

class MouseMovePlatformEvent : public Event {
public:
    DEFINE_EVENT_TYPE(MouseMovePlatformEvent);

    MouseMovePlatformEvent(ImGuiViewport* viewport, glm::vec2 position)
        : viewport(viewport), position(position) {}

    ImGuiViewport* viewport;
    glm::vec2 position{ .0f, .0f };
};

class MouseScrollPlatformEvent : public Event {
public:
    DEFINE_EVENT_TYPE(MouseScrollPlatformEvent);

    MouseScrollPlatformEvent(ImGuiViewport* viewport, glm::vec2 offset)
        : viewport(viewport), offset(offset) {}

    ImGuiViewport* viewport;
    glm::vec2 offset{ .0f, .0f };
};

} // namespace dirk::Platform
