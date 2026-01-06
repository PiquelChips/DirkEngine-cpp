#pragma once

#include "Events/Event.hpp"

#include "imgui.h"
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

} // namespace dirk::Platform
