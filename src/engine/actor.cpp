#include "engine/actor.hpp"
#include "engine/dirkengine.hpp"
#include "render/render_types.hpp"
#include "render/render_utils.hpp"
#include "render/vulkan_types.hpp"

#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"

#include <cstring>

namespace dirk {

Actor::Actor(ActorCreateInfo& spawnInfo)
    : name(spawnInfo.name),
      engine(spawnInfo.engine),
      transform(spawnInfo.transform),
      transformMatrix(transform.getMatrix()) {
    setModel(spawnInfo.modelName);
}

void Actor::setModel(const std::string_view name) {
    this->model = engine->getResourceManager()->loadModel(std::string(name));
    updateData();
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

void Actor::updateData() {
    auto [device, physicalDevice] = engine->getRenderer()->getDevices();
    Queues queues = engine->getRenderer()->getQueues();
    vk::SurfaceKHR surface = engine->getRenderer()->getSurface();

    vk::CommandBuffer commandBuffer = RenderUtils::beginSingleTimeCommands(device, physicalDevice, surface);

    // ubo
    {
        vk::DeviceSize bufferSize = sizeof(ModelViewProjection);
        auto [buffer, bufferMemory] = RenderUtils::createBuffer(
            device, physicalDevice, bufferSize,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        uniformBuffer = buffer;
        uniformBufferMemory = bufferMemory;
        uniformBufferMapped = device.mapMemory(uniformBufferMemory, 0, bufferSize);
    }

    // vertex buffer
    {
        vk::DeviceSize bufferSize = sizeof(model->vertices[0]) * model->vertices.size();

        auto [stagingBuffer, stagingBufferMemory] = RenderUtils::createBuffer(
            device, physicalDevice, bufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        void* dataStaging = device.mapMemory(stagingBufferMemory, 0, bufferSize);
        memcpy(dataStaging, model->vertices.data(), bufferSize);
        device.unmapMemory(stagingBufferMemory);

        auto [buffer, bufferMemory] = RenderUtils::createBuffer(
            device, physicalDevice, bufferSize,
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal);
        vertexBuffer = buffer;
        vertexBufferMemory = bufferMemory;

        commandBuffer.copyBuffer(stagingBuffer, vertexBuffer, vk::BufferCopy(0, 0, bufferSize));
    }

    // index buffer
    {
        vk::DeviceSize bufferSize = sizeof(model->indices[0]) * model->indices.size();

        auto [stagingBuffer, stagingBufferMemory] = RenderUtils::createBuffer(
            device, physicalDevice, bufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        void* data = device.mapMemory(stagingBufferMemory, 0, bufferSize);
        memcpy(data, model->indices.data(), bufferSize);
        device.unmapMemory(stagingBufferMemory);

        auto [buffer, bufferMemory] = RenderUtils::createBuffer(
            device, physicalDevice, bufferSize,
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal);
        indexBuffer = buffer;
        indexBufferMemory = bufferMemory;

        commandBuffer.copyBuffer(stagingBuffer, indexBuffer, vk::BufferCopy(0, 0, bufferSize));
    }

    // texture
    {
        const Texture& texture = model->texture;
        vk::DeviceSize imageSize = texture.size;

        mipLevels = std::floor(std::log2(std::max(texture.width, texture.height))) + 1;

        vk::Format format = vk::Format::eR8G8B8A8Srgb;

        auto [stagingBuffer, stagingBufferMemory] = RenderUtils::createBuffer(
            device, physicalDevice, imageSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        void* data = device.mapMemory(stagingBufferMemory, 0, imageSize);
        memcpy(data, texture.texture.data(), imageSize);
        device.unmapMemory(stagingBufferMemory);

        CreateImageMemoryViewInfo createInfo{
            .device = device,
            .physicalDevice = physicalDevice,
            .width = static_cast<uint32_t>(model->texture.width),
            .height = static_cast<uint32_t>(model->texture.height),
            .format = model->texture.format,
            .tiling = vk::ImageTiling::eOptimal,
            .usage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
            .properties = vk::MemoryPropertyFlagBits::eDeviceLocal,
            .mipLevels = mipLevels,
        };
        textureImageMemoryView = RenderUtils::createImageMemoryView(createInfo);

        RenderUtils::transitionImageLayout(commandBuffer, textureImageMemoryView.image, format, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, mipLevels);
        RenderUtils::copyBufferToImage(commandBuffer, stagingBuffer, textureImageMemoryView.image, texture.width, texture.height);
        RenderUtils::generateMipmaps(commandBuffer, physicalDevice, textureImageMemoryView.image, format, texture.width, texture.height, mipLevels);
        // transitions to vk::ImageLayout::eShaderReadOnlyOptimal while generating mipmaps
    }

    // sampler
    {
        vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
        vk::SamplerCreateInfo samplerInfo{};

        samplerInfo.magFilter = vk::Filter::eLinear;
        samplerInfo.minFilter = vk::Filter::eLinear;

        samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
        samplerInfo.mipLodBias = 0.f;
        samplerInfo.minLod = 0.f;
        samplerInfo.maxLod = vk::LodClampNone;

        samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
        samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
        samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;

        samplerInfo.anisotropyEnable = engine->getRenderer()->getFeatures().anisotropy ? vk::True : vk::False;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

        samplerInfo.compareEnable = vk::False;
        samplerInfo.compareOp = vk::CompareOp::eAlways;
        samplerInfo.unnormalizedCoordinates = vk::False; // the tex coords are normalized

        textureSampler = device.createSampler(samplerInfo);
    }

    descriptorSet = engine->getRenderer()->createDescriptorSets(uniformBuffer, textureSampler, textureImageMemoryView.view, vk::ImageLayout::eShaderReadOnlyOptimal);

    RenderUtils::endSingleTimeCommands(commandBuffer, queues.graphicsQueue);
}

void Actor::recordCommandBuffer(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout) {
    ModelViewProjection mvp{
        .model = transformMatrix,
        .view = glm::lookAt(glm::vec3(0.f, 200.f, 200.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 1.f)), // TDOO: get from player location in the future
        .proj = engine->getRenderer()->getProjection(),
    };
    std::memcpy(uniformBufferMapped, &mvp, sizeof(mvp));

    commandBuffer.bindVertexBuffers(0, vertexBuffer, { 0 });
    commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSet, nullptr);

    commandBuffer.drawIndexed(model->indices.size(), 1, 0, 0, 0);
}

} // namespace dirk
