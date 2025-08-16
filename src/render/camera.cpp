#include "render/camera.hpp"

#include <cstdint>

namespace dirk {

DEFINE_LOG_CATEGORY(LogCamera);

Camera::Camera(glm::vec3 position, glm::vec3 direction, float fov, float nearClip, float farClip)
    : position(position), direction(direction), fov(fov), nearClip(nearClip), farClip(farClip) {
    updateProjection();
    updateView();
}

void Camera::tick(float deltaTime) {}

void Camera::resize(std::uint32_t width, std::uint32_t height) {}

void Camera::updateProjection() {}
void Camera::updateView() {}

} // namespace dirk
