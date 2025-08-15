#include "engine/actor.hpp"
#include "engine/dirkengine.hpp"
#include "render/vulkan_types.hpp"

namespace dirk {

Actor::Actor(ActorCreateInfo& spawnInfo)
    : name(spawnInfo.name),
      engine(spawnInfo.engine),
      transform(spawnInfo.transform),
      transformMatrix(transform.getMatrix()) {
    // register model & tex to renderer
}

Actor::~Actor() {
    // remove model & tex from renderer
}

void Actor::tick(float deltaTime) {}

void Actor::destroy() {
    engine->destroyActor(this);
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

void Actor::updateActorModel(const std::string_view name) {}

vk::DescriptorSetLayout createDescriptorSetLayout() { return nullptr; }
vk::DescriptorPool createDescriptorPool() { return nullptr; }
ImageMemoryView createTextureResources() { return ImageMemoryView(); }
vk::Sampler createTextureSampler() { return nullptr; }
vk::Buffer createVertexBuffer() { return nullptr; }
vk::Buffer createIndexBuffer() { return nullptr; }

} // namespace dirk
