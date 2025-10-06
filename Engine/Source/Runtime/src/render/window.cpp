#include "render/window.hpp"

namespace dirk {

Window::Window(const WindowCreateInfo& createInfo) {
    // TODO: window constructor
}

Window::~Window() {
    // TODO: window destructor
}

// TODO: implement window functions
void Window::addViewport(ViewportId id, vk::Rect2D region, int order) {}
void Window::removeViewport(ViewportId id) {}
void Window::updateViewportRegion(ViewportId id, vk::Rect2D newRegion) {}
void Window::setViewportRenderOrder(ViewportId id, int order) {}
void Window::setViewportEnabled(ViewportId id, bool enabled) {}

void Window::processPlatformEvents() {
    platformWindow->pollEvents();
    // TODO: process platform events
}

} // namespace dirk
