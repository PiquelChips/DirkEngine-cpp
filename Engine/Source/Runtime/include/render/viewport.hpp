#include "glm/glm.hpp"
#include "vulkan/vulkan.hpp"

#include <memory>

#pragma once

// representation of a viewport of the engine (essentially a renderer output)
namespace dirk {

typedef std::uint16_t ViewportId;

struct ViewportCreateInfo {};

class Camera;

class Viewport {
public:
    Viewport(const ViewportCreateInfo& createInfo);
    ~Viewport();

    std::shared_ptr<Camera> getCamera() { return camera; }

    vk::Extent2D getSize() const { return size; }

    void setClearColor(glm::vec3 color);
    glm::vec3 getClearColor() const { return clearColor; }

    static glm::vec2 screenToViewport(glm::vec2 screenPos, vk::Rect2D viewportRegion);
    static glm::vec2 viewportToScreen(glm::vec2 veiwportPos, vk::Rect2D viewportRegion);

private:
    vk::Extent2D size;
    std::shared_ptr<Camera> camera;
    glm::vec3 clearColor;
};

} // namespace dirk
