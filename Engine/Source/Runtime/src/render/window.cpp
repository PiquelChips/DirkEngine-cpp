#include "render/window.hpp"
#include "render/renderer.hpp"
#include <memory>

namespace dirk {

Window::Window(const WindowCreateInfo& createInfo) {
    // TODO: create platform window
}

Window::~Window() {
    // TODO: destroy platform window
}

void Window::addViewport(ViewportId id, vk::Rect2D region, int order) {
    viewportAssignements[id] = ViewportAssignement(id, region, order, true);
}

void Window::removeViewport(ViewportId id) {
    viewportAssignements.erase(id);
}

void Window::updateViewportRegion(ViewportId id, vk::Rect2D newRegion) {
    viewportAssignements.at(id).region = newRegion;
}

void Window::setViewportRenderOrder(ViewportId id, int order) {
    viewportAssignements.at(id).renderOrder = order;
}

void Window::setViewportEnabled(ViewportId id, bool enabled) {
    viewportAssignements.at(id).enabled = enabled;
}

void Window::processPlatformEvents() {
    platformWindow->pollEvents();
    // TODO: process platform events
}

} // namespace dirk
