#include "engine/actor.hpp"
#include "engine/dirkengine.hpp"
#include "render/render_types.hpp"

namespace dirk {

Actor::Actor(ActorSpawnInfo& spawnInfo)
    : name(spawnInfo.name),
      engine(spawnInfo.engine),
      transform(spawnInfo.transform),
      transformMatrix(transform.getMatrix()) {}

void Actor::destroy() {
    deinitialize();
}

void Actor::setTransform(const Transform& inTransform) {
    this->transform = inTransform;
    updateTransformMatrix();
}
void Actor::setLocation(const glm::vec3& inLocation) {
    this->transform.location = inLocation;
    updateTransformMatrix();
}
void Actor::setRotation(const glm::vec3& inRotation) {
    this->transform.rotation = inRotation;
    updateTransformMatrix();
}
void Actor::setScale(const glm::vec3& inScale) {
    this->transform.scale = inScale;
    updateTransformMatrix();
}

void Actor::updateTransformMatrix() {
    transformMatrix = transform.getMatrix();
}

} // namespace dirk
