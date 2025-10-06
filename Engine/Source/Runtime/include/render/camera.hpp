#pragma once

#include "core/globals.hpp"

#include "glm/glm.hpp"
#include "render/viewport.hpp"

#include <cstdint>
#include <memory>

namespace dirk {

DECLARE_LOG_CATEGORY_EXTERN(LogCamera)

class Camera {
public:
    Camera(glm::vec3 positon, glm::vec3 forwardDirection, float fov, float nearClip, float farClip);

    void tick(float deltaTime);
    void resize(std::uint32_t width, std::uint32_t height);

    inline const glm::mat4& getProjection() const { return projection; }
    inline const glm::mat4& getInverseProjection() const { return inverseProjection; }
    inline const glm::mat4& getView() const { return view; }
    inline const glm::mat4& getInverseView() const { return inverseView; }

private:
    void updateProjection();
    void updateView();

    glm::mat4 projection{ 1.f };
    glm::mat4 inverseProjection{ 1.f };
    glm::mat4 view{ 1.f };
    glm::mat4 inverseView{ 1.f };

    // camera settings
    float fov = glm::radians(45.f);
    float nearClip = .1f;
    float farClip = 100.f;

    glm::vec3 position{ 0.f };
    glm::vec3 forwardDirection{ 0.f };

    glm::vec2 lastMousePosition{ 0.f };
    std::uint32_t width, height;

    std::shared_ptr<Viewport> viewport;

    static constexpr glm::vec3 upDirection{ 0.f, 1.f, 0.f };
    static constexpr float SENSITIVITY = .002f;
    static constexpr float ROTATION_SPEED = .3f;
    static constexpr float MOVEMENT_SPEED = 1000.f;
};

} // namespace dirk
