#pragma once

#include "core.hpp"

#include "engine/world.hpp"
#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "render/viewport.hpp"
#include "vulkan/vulkan_structs.hpp"

namespace dirk {

DECLARE_LOG_CATEGORY_EXTERN(LogCamera)

struct CameraCreateInfo {
    glm::vec3 positon;
    glm::vec3 forwardDirection;
    float fov;
    float nearClip;
    float farClip;
};

class Camera {
public:
    Camera(const CameraCreateInfo& createInfo, Viewport& viewport);

    void tick(float deltaTime);
    void resize(vk::Extent2D inSize);

    void addMoveInput(glm::vec3 move);
    void addLookInput(glm::vec2 look);

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
    vk::Extent2D size;

    // camera is owned by the viewport
    Viewport& viewport;

    // state
    glm::vec3 cachedMoveInput = { 0.f, 0.f, 0.f };
    glm::vec2 cachedLookInput = { 0.f, 0.f };

    static constexpr float SENSITIVITY = .002f;
    static constexpr float ROTATION_SPEED = .3f;
    static constexpr float MOVEMENT_SPEED = 50000.f;
};

} // namespace dirk
