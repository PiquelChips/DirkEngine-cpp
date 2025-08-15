#pragma once

#include "render/render_types.hpp"
#include "render/vulkan_types.hpp"

#include "glm/glm.hpp"

#include <memory>
#include <string_view>

namespace dirk {

class DirkEngine;

struct ActorCreateInfo {
    const std::string_view name;
    const std::string_view modelName;
    DirkEngine* engine;
    Transform transform;
};

/**
 * Just the representation of something in the game world
 */
class Actor {

public:
    Actor(ActorCreateInfo& spawnInfo);
    ~Actor();

    void tick(float deltaTime);
    void destroy();

    inline const std::string_view getName() const { return name; }
    inline const glm::mat4& getTransformMatrix() const { return transformMatrix; }
    inline void setVisibity(bool inVisible) { visible = inVisible; }
    inline bool isVisible() { return visible; }

private:
    DirkEngine* engine;
    const std::string_view name;
    bool visible = false;

public:
    inline const Transform& getTransform() const { return transform; }
    inline void setTransform(const Transform& inTransform);

    inline const glm::vec3& getLocation() const { return transform.location; }
    inline void setLocation(const glm::vec3& inLocation);

    inline const glm::vec3& getRotation() const { return transform.rotation; }
    inline void setRotation(const glm::vec3& inRotation);

    inline const glm::vec3& getScale() const { return transform.scale; }
    inline void setScale(const glm::vec3& inScale);

private:
    void updateTransformMatrix();
    Transform transform;
    glm::mat4 transformMatrix{ 1.f };

public:
    inline const std::string_view getModelName() const noexcept { return model->name; }
    void updateActorModel(const std::string_view name);

private:
    vk::DescriptorSetLayout createDescriptorSetLayout();
    vk::DescriptorPool createDescriptorPool();
    ImageMemoryView createTextureResources();
    vk::Sampler createTextureSampler();

    // vertices & indices
    vk::Buffer createVertexBuffer();
    vk::Buffer createIndexBuffer();

    vk::DescriptorSetLayout descriptorSetLayout;
    vk::DescriptorPool descriptorPool;

    std::shared_ptr<const Model> model;

    vk::Buffer vertexBuffer;
    vk::DeviceMemory vertexBufferMemory;

    vk::Buffer indexBuffer;
    vk::DeviceMemory indexBufferMemory;

    ImageMemoryView textureImageMemoryView;

    uint32_t mipLevels;
    vk::Sampler textureSampler;
};

} // namespace dirk
