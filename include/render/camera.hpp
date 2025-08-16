#pragma once

#include "core/globals.hpp"

#include "engine/dirkengine.hpp"
#include "glm/glm.hpp"
#include <cstdint>
#include <memory>

namespace dirk {

DECLARE_LOG_CATEGORY_EXTERN(LogCamera)

class Camera {
public:
    Camera(glm::vec3 positon, glm::vec3 direction, float fov, float nearClip, float farClip);

    void tick(float deltaTime);
    void resize(std::uint32_t width, std::uint32_t height);

    inline const glm::mat4& getProjection() const { return projection; }
    inline const glm::mat4& getInverseProjection() const { return inverseProjection; }
    inline const glm::mat4& getView() const { return view; }
    inline const glm::mat4& getInverseView() const { return inverseView; }

    static std::shared_ptr<Camera> get() { return DirkEngine::getCamera(); }

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
    glm::vec3 direction{ 0.f };

    std::uint32_t width, height;
};

} // namespace dirk
