#include "render/camera.hpp"

#include "glm/gtc/matrix_transform.hpp"

namespace dirk {

DEFINE_LOG_CATEGORY(LogCamera);

Camera::Camera() { updateProjectionMatrix(); }

void Camera::tick(float deltaTime) {}

void Camera::setFieldOfView(float inFieldOfView) {
    fieldOfView = inFieldOfView;
    updateProjectionMatrix();
}
void Camera::setAspectRatio(float inAspectRation) {
    aspectRatio = inAspectRation;
    updateProjectionMatrix();
}
void Camera::setNear(float inNear) {
    near = inNear;
    updateProjectionMatrix();
}
void Camera::setFar(float inFar) {
    far = inFar;
    updateProjectionMatrix();
}

void Camera::updateProjectionMatrix() {
    projectionMatrix = glm::perspective(fieldOfView, aspectRatio, near, far);
    // projectionMatrix[1][1] *= -1;
}

} // namespace dirk
