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

    std::shared_ptr<Camera> getCamera();

    vk::Extent2D getSize();

    void setClearColor(glm::vec3 color);
    void setClearDepth(float depth);
    glm::vec3 getClearColor() const;

    static glm::vec2 screenToViewport(glm::vec2 screenPos, vk::Rect2D viewportRegion);
    static glm::vec2 viewportToScreen(glm::vec2 veiwportPos, vk::Rect2D viewportRegion);

private:
    std::shared_ptr<Camera> camera;
    glm::vec3 clearColor;
    float clearDepth;
};

} // namespace dirk
