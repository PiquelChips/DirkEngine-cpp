#include "render/camera.hpp"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "vulkan/vulkan_structs.hpp"

#include "input/input.hpp"
#include "input/keys.hpp"
#include "render/renderer.hpp"

#include <cstdint>

namespace dirk {

DEFINE_LOG_CATEGORY(LogCamera);

Camera::Camera(glm::vec3 position, glm::vec3 forwardDirection, float fov, float nearClip, float farClip)
    : position(position), forwardDirection(glm::normalize(forwardDirection)), fov(fov), nearClip(nearClip), farClip(farClip) {

    vk::Extent2D extent = Renderer::get()->getSwapChainExtent();
    width = extent.width;
    height = extent.height;

    updateProjection();
    updateView();
}

void Camera::tick(float deltaTime) {
    glm::vec2 mousePos = Input::getMousePosition();
    glm::vec2 delta = (mousePos - lastMousePosition) * SENSITIVITY;
    lastMousePosition = mousePos;

    if (!Input::isMouseButtonDown(MouseButton::Right)) {
        Input::setCursorMode(CursorMode::Normal);
        return;
    }
    Input::setCursorMode(CursorMode::Locked);

    glm::vec3 rightDirection = glm::normalize(glm::cross(forwardDirection, upDirection));

    bool moved = false;
    // movement
    glm::vec3 moveDirection(0.0f);

    if (Input::isKeyDown(Key::W)) {
        moveDirection += forwardDirection;
    }
    if (Input::isKeyDown(Key::S)) {
        moveDirection -= forwardDirection;
    }
    if (Input::isKeyDown(Key::D)) {
        moveDirection += rightDirection;
    }
    if (Input::isKeyDown(Key::Q)) {
        moveDirection -= rightDirection;
    }
    if (Input::isKeyDown(Key::Space)) {
        moveDirection += upDirection;
    }
    if (Input::isKeyDown(Key::C)) {
        moveDirection -= upDirection;
    }

    if (glm::length(moveDirection) > 0.0f) {
        moveDirection = glm::normalize(moveDirection);
        position += moveDirection * MOVEMENT_SPEED * deltaTime;
        moved = true;
    }

    // rotation
    if (delta.x != 0.f || delta.y != 0.f) {
        float yawDelta = delta.x * ROTATION_SPEED;
        float pitchDelta = delta.y * ROTATION_SPEED;

        glm::quat q = glm::normalize(glm::cross(
            glm::angleAxis(-pitchDelta, rightDirection),
            glm::angleAxis(-yawDelta, upDirection)));
        forwardDirection = glm::rotate(q, forwardDirection);

        moved = true;
    }

    if (moved) {
        updateView();
    }
}

void Camera::resize(std::uint32_t width, std::uint32_t height) {
    this->width = width;
    this->height = height;

    updateProjection();
}

void Camera::updateProjection() {
    projection = glm::perspectiveFov(fov, (float) width, (float) height, nearClip, farClip);
    projection[1][1] *= -1;
    inverseProjection = glm::inverse(projection);
}

void Camera::updateView() {
    view = glm::lookAt(position, position + forwardDirection, upDirection);
    inverseView = glm::inverse(view);
}

} // namespace dirk
