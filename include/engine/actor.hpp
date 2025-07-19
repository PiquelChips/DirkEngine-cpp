#pragma once

#include "render/render_types.hpp"

#include "glm/glm.hpp"

#include <string>
#include <vector>

namespace dirk {

class DirkEngine;
class Component;

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
    void deinitialize();

    const std::string& getName() const { return name; }

    // begin Actor interface
public:
    virtual void beginPlay();
    virtual void tick(float deltaTime);
    virtual void endPlay();
    // end Actor interface

    inline const Transform& getTransform() const { return transform; }
    inline void setTransform(const Transform& transform);

    inline const glm::vec3& getLocation() const { return transform.location; }
    inline void setLocation(const glm::vec3& inLocation);

    inline const glm::vec3& getRotation() const { return transform.rotation; }
    inline void setRotation(const glm::vec3& inRotation);

    inline const glm::vec3& getScale() const { return transform.scale; }
    inline void setScale(const glm::vec3& inScale);

private:
    Transform transform;
    DirkEngine* engine;
    const std::string& name;

} // namespace dirk
