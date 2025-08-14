#include "render/render_types.hpp"
#include "render/vulkan_types.hpp"

namespace dirk {

RendererCreateInfo::operator RendererProperties() {
    return RendererProperties{
        .applicationName = applicationName,
        .windowWidth = windowWidth,
        .windowHeight = windowHeight,
        .engine = engine,
    };
};

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
