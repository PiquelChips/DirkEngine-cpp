#include "render/viewport.hpp"
#include "render/camera.hpp"

namespace dirk {

Viewport::Viewport(const ViewportCreateInfo& createInfo) {
    // TODO: viewport constructor
}

Viewport::~Viewport() {
    // TODO: viewport destructor
}

void Viewport::setClearColor(glm::vec3 color) {
    this->clearColor = color;
}

glm::vec2 Viewport::screenToViewport(glm::vec2 screenPos, vk::Rect2D viewportRegion) {
    // TODO: viewport::screenToViewport
    return glm::vec2(0.f);
}

glm::vec2 Viewport::viewportToScreen(glm::vec2 veiwportPos, vk::Rect2D viewportRegion) {
    // TODO: viewport::viewportToScreen
    return glm::vec2(0.f);
}

} // namespace dirk
