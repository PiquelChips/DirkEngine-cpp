#include "render/renderer.hpp"
#include "render/renderer_types.hpp"
#include "vulkan/vulkan.hpp"
#include <memory>

namespace dirk {

DEFINE_LOG_CATEGORY(LogRenderer)

RendererCreateInfo::operator RendererProperties() {
    return RendererProperties{
        .applicationName = applicationName,
        .windowWidth = windowWidth,
        .windowHeight = windowHeight,
        .api = api,
        .engine = engine,
    };
};

std::unique_ptr<Renderer> createRenderer(RendererCreateInfo& createInfo) {
    switch (createInfo.api) {
    case Vulkan:
        return std::make_unique<VulkanRenderer>(createInfo);
    }

    DIRK_LOG(LogRenderer, FATAL, "an invalid api was specified in RendererCreateInfo.api");
    return nullptr;
}

glm::mat4 Transform::getMatrix() {
    glm::mat4 matrix = glm::mat4(1.f);

    matrix = glm::translate(matrix, location);

    matrix = glm::scale(matrix, scale);

    matrix = glm::rotate(matrix, glm::radians(rotation.x), glm::vec3(0.f, 1.f, 0.f));
    matrix = glm::rotate(matrix, glm::radians(rotation.y), glm::vec3(1.f, 0.f, 0.f));
    matrix = glm::rotate(matrix, glm::radians(rotation.z), glm::vec3(0.f, 0.f, 1.f));

    return matrix;
}

} // namespace dirk
