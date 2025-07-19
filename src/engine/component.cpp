#include "engine/component.hpp"

#include "engine/actor.hpp"

namespace dirk {

Component::Component(ComponentCreateInfo& createInfo)
    : name(createInfo.name), owningActor(createInfo.owningActor) {}

void Component::setTransform(const Transform& inTransform) {
    this->transform = inTransform;
    updateTransformMatrix();
}
void Component::setLocation(const glm::vec3& inLocation) {
    this->transform.location = inLocation;
    updateTransformMatrix();
}
void Component::setRotation(const glm::vec3& inRotation) {
    this->transform.rotation = inRotation;
    updateTransformMatrix();
}
void Component::setScale(const glm::vec3& inScale) {
    this->transform.scale = inScale;
    updateTransformMatrix();
}

void Component::updateTransformMatrix() {
    transformMatrix = transform.getMatrix() * getOwningActor()->getTransformMatrix();
}

} // namespace dirk
