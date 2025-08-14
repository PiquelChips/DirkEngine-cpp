#include "core/globals.hpp"
#include "engine/actor.hpp"

#include "vulkan/vulkan_handles.hpp"
#include "vulkan_types.hpp"

namespace dirk {

/**
 * The physical representation of a shape in the world.
 * Must be owned by an actor.
 * Will be overriden by various renderer subclasses for API
 * specific functionnality.
 */
class Mesh {
public:
    inline Actor* getOwningActor() { return owningActor; }

    inline virtual void setVisibity(bool inVisible) { isVisible = inVisible; }
    inline virtual bool visible() { return isVisible; }

    const Transform& getTransform() { return getOwningActor()->getTransform(); }
    const glm::mat4& getTransformMatrix() { return getOwningActor()->getTransformMatrix(); }

private:
    Actor* owningActor;

    bool isVisible = false;

    Vulkan(Actor* owningActor);

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
