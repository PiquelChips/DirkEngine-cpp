#include "render/camera.hpp"

#include "glm/gtx/quaternion.hpp"
#include "render/renderer.hpp"
#include "vulkan/vulkan_structs.hpp"

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
    if (glm::length(cachedMoveInput) > 0.0f) {
        glm::vec3 move = (rightDirection * cachedMoveInput.x) + (UP_DIRECTION * cachedMoveInput.y) + (forwardDirection * cachedMoveInput.z);
        position += move * MOVEMENT_SPEED * deltaTime;
        moved = true;
        cachedMoveInput = glm::vec3(0.f);
    }

    // rotation
    if (cachedLookInput != glm::vec2{ 0.f, 0.f }) {
        cachedLookInput *= SENSITIVITY * ROTATION_SPEED;

        float yawDelta = cachedLookInput.x;
        float pitchDelta = cachedLookInput.y;

        glm::quat q = glm::normalize(glm::cross(
            glm::angleAxis(-pitchDelta, rightDirection),
            glm::angleAxis(-yawDelta, UP_DIRECTION)));
        forwardDirection = glm::normalize(glm::rotate(q, forwardDirection));

        moved = true;
        cachedLookInput = glm::vec3(0.f);
    }

    if (moved) {
        updateView();
    }

    // TODO: setup mouse capture
    /**
    if (!Input::isMouseButtonDown(Input::MouseButton::Right)) {
        Input::setCursorMode(CursorMode::Normal);
        return;
    }
    Input::setCursorMode(CursorMode::Locked);
    */
}

void Camera::addMoveInput(glm::vec3 move) {
    cachedMoveInput = glm::normalize(move);
}

void Camera::addLookInput(glm::vec2 look) {
    cachedLookInput = look;
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
