#pragma once

#include "render/vulkan_types.hpp"

#include "glm/glm.hpp"

#include <memory>
#include <string>
#include <vector>

namespace dirk {

class DirkEngine;

struct ActorSpawnInfo {
    const std::string& name;
    DirkEngine* engine;
    Transform transform;
};

/**
 * Just the representation of something in the game world
 *
 * TODO: create a world class to manage this stuff (instead of engine)
 */
class Actor {

public:
    Actor(ActorSpawnInfo& spawnInfo);

    void destroy();

    const std::string& getName() const { return name; }
    const glm::mat4& getTransformMatrix() const { return transformMatrix; }

    // begin Actor interface
    virtual void initialize() = 0;
    virtual void tick(float deltaTime) = 0;
    virtual void deinitialize() = 0;
    // end Actor interface

    inline const Transform& getTransform() const { return transform; }
    inline void setTransform(const Transform& inTransform);

    inline const glm::vec3& getLocation() const { return transform.location; }
    inline void setLocation(const glm::vec3& inLocation);

    inline const glm::vec3& getRotation() const { return transform.rotation; }
    inline void setRotation(const glm::vec3& inRotation);

    inline const glm::vec3& getScale() const { return transform.scale; }
    inline void setScale(const glm::vec3& inScale);

    inline DirkEngine* getEngine() const noexcept { return engine; }

private:
    void updateTransformMatrix();
    Transform transform;
    glm::mat4 transformMatrix{ 1.f };

    DirkEngine* engine;
    const std::string& name;
};

} // namespace dirk
