#pragma once

#include "core/globals.hpp"

#include "glm/glm.hpp"

namespace dirk {

DECLARE_LOG_CATEGORY_EXTERN(LogCamera)

class Camera {
public:
    Camera();

    void tick(float deltaTime);

    inline const glm::mat4 getProjectionMatrix() const noexcept { return projectionMatrix; }

    inline float getFieldOfView() { return fieldOfView; }
    void setFieldOfView(float inFieldOfView);

    inline float getAspectRatio() { return aspectRatio; }
    void setAspectRatio(float inAspectRatio);

    inline float getNear() { return near; }
    void setNear(float inNear);

    inline float getFar() { return far; }
    void setFar(float inFar);

private:
    void updateProjectionMatrix();
    glm::mat4 projectionMatrix{ 1.f };

    // camera settings
    float fieldOfView = glm::radians(90.f);
    float aspectRatio = 16.f / 9.f;
    float near = .0001;
    float far = 1000.f;
};

} // namespace dirk
