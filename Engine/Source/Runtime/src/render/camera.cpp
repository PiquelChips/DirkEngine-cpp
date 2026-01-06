#include "render/camera.hpp"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "render/renderer.hpp"
#include "vulkan/vulkan_structs.hpp"

#include <cstdint>

namespace dirk {

DEFINE_LOG_CATEGORY(LogCamera);

Camera::Camera(const CameraCreateInfo& createInfo, Viewport& viewport)
    : viewport(viewport),
      size(viewport.getSize()),
      position(createInfo.positon),
      forwardDirection(glm::normalize(createInfo.forwardDirection)),
      fov(createInfo.fov),
      nearClip(createInfo.nearClip),
      farClip(createInfo.farClip) {

    updateProjection();
    updateView();
}

void Camera::tick(float deltaTime) {
    bool moved = false;
    glm::vec3 rightDirection = glm::normalize(glm::cross(forwardDirection, UP_DIRECTION));
    if (glm::length(cachedMove) > 0.0f) {
        cachedMove = glm::normalize(cachedMove);
        position += cachedMove * forwardDirection * MOVEMENT_SPEED * deltaTime;
        moved = true;
    }

    // rotation
    if (cachedLook != glm::vec2{ 0.f, 0.f }) {
        cachedLook *= SENSITIVITY * ROTATION_SPEED;

        float yawDelta = cachedLook.x;
        float pitchDelta = cachedLook.y;

        glm::quat q = glm::normalize(glm::cross(
            glm::angleAxis(-pitchDelta, rightDirection),
            glm::angleAxis(-yawDelta, UP_DIRECTION)));
        forwardDirection = glm::rotate(q, forwardDirection);

        moved = true;
    }

    if (moved) {
        DIRK_LOG(LogCamera, TRACE, "updated view")
        updateView();
    }

    /**
    if (!Input::isMouseButtonDown(Input::MouseButton::Right)) {
        Input::setCursorMode(CursorMode::Normal);
        return;
    }
    Input::setCursorMode(CursorMode::Locked);
    */
}

void Camera::addMoveInput(glm::vec3 move) {
    cachedMove = move;
}

void Camera::addLookInput(glm::vec2 look) {
    cachedLook = look;
}

void Camera::resize(vk::Extent2D inSize) {
    this->size = inSize;

    updateProjection();
}

void Camera::updateProjection() {
    projection = glm::perspectiveFov(fov, (float) size.width, (float) size.height, nearClip, farClip);
    projection[1][1] *= -1;
    inverseProjection = glm::inverse(projection);
}

void Camera::updateView() {
    view = glm::lookAt(position, position + forwardDirection, UP_DIRECTION);
    inverseView = glm::inverse(view);
}

} // namespace dirk
