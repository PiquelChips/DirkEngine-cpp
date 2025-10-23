#pragma once

// GLM
#include <memory>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES // properly aligns types in memory; this doesn't affect nested structs!!!
#define GLM_FORCE_DEPTH_ZERO_TO_ONE        // vulkan uses the 0.0 to 1.0 ranges; opengl uses the -1.0 to 1.0 range
#define GLM_ENABLE_EXPERIMENTAL            // for the glm hash functions
#include "glm/glm.hpp"
#include "vulkan/vulkan.hpp"

#include "asserts.hpp"
#include "logging/logging.hpp"

namespace dirk {
class IEngine;
extern IEngine* gEngine;

struct Queues {
    vk::Queue graphicsQueue;
    vk::Queue presentQueue;
};

struct RendererResources {
    vk::Instance instance;
    vk::PhysicalDevice physicalDevice;
    vk::Device device;

    Queues queues;
    vk::CommandPool commandPool;
    vk::DescriptorSetLayout descriptorSetLayout;
};

struct RendererProperties {
    vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;
    bool anisotropy = false;
    vk::Format swapChainImageFormat = vk::Format::eUndefined;
    std::uint32_t minImageCount;
};

struct DeviceFeatures {
    bool anisotropy = false;
    vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;

    bool isComplete() {
        return anisotropy && static_cast<int>(msaaSamples) > 1;
    }

    int getScore() {
        if (isComplete())
            return 1000;

        int score = 0;

        if (anisotropy)
            score += 10;

        score += static_cast<int>(msaaSamples);

        return score;
    }
};

struct SwapChainCreateInfo {
    // OUTPUT
    vk::SwapchainKHR& swapChain; // the output swapchain
    vk::Format& swapChainImageFormat;
    vk::Extent2D& swapChainExtent;

    // INPUT
    vk::RenderPass renderPass;
    vk::SurfaceKHR surface;
    vk::Extent2D windowSize;
};

struct SwapChainImage {
    vk::ImageView imageView;
    vk::Framebuffer frameBuffer;

    operator bool() const { return imageView && frameBuffer; }
};

struct ImageMemoryView {
    vk::Image image;
    vk::DeviceMemory memory;
    vk::ImageView view;

    operator bool() const { return image && memory && view; }
};

struct CreateImageMemoryViewInfo {
    // the image
    uint32_t width, height;
    vk::Format format;
    vk::ImageTiling tiling;
    vk::ImageUsageFlags usage;
    // the memory
    vk::MemoryPropertyFlags properties;
    // the view
    vk::ImageAspectFlags imageAspect = vk::ImageAspectFlagBits::eColor;
    // MSAA & mipmaps
    vk::SampleCountFlagBits numSamples = vk::SampleCountFlagBits::e1;
    uint32_t mipLevels = 1;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual std::vector<SwapChainImage> createSwapChain(const SwapChainCreateInfo& createInfo) = 0;
    virtual vk::ShaderModule loadShaderModule(const std::string& shaderName) = 0;
    virtual vk::Semaphore createSemaphore() = 0;
    virtual vk::DescriptorSet createDescriptorSets(vk::Buffer uniformBuffer, vk::Sampler sampler, vk::ImageView imageView, vk::ImageLayout layout) = 0;

    virtual RendererResources getResources() = 0;
    virtual const RendererProperties& getProperties() = 0;
    virtual const DeviceFeatures getDeviceFeatures() = 0;

    // some utility functions
    virtual ImageMemoryView createImageMemoryView(CreateImageMemoryViewInfo& createInfo) = 0;

    virtual std::tuple<vk::Image, vk::DeviceMemory> createImage(
        uint32_t width, uint32_t height, vk::Format format,
        vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties,
        vk::SampleCountFlagBits numSamples = vk::SampleCountFlagBits::e1,
        uint32_t mipLevels = 1) = 0;

    virtual vk::ImageView createImageView(vk::Image& image, vk::Format format, vk::ImageAspectFlags imageAspect = vk::ImageAspectFlagBits::eColor, uint32_t mipLevels = 1) = 0;

    virtual std::tuple<vk::Buffer, vk::DeviceMemory> createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) = 0;
    virtual QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device) = 0;

    virtual vk::CommandBuffer beginSingleTimeCommands() = 0;
    virtual void endSingleTimeCommands(vk::CommandBuffer& commandBuffer, vk::Queue queue) = 0;

    virtual void transitionImageLayout(vk::CommandBuffer commandBuffer, const vk::Image& image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels = 1) = 0;
    virtual void copyBuffer(vk::CommandBuffer commandBuffer, vk::Buffer& srcBuffer, vk::Buffer& dstBuffer, vk::DeviceSize size) = 0;
    virtual void copyBufferToImage(vk::CommandBuffer commandBuffer, vk::Buffer& buffer, vk::Image& image, uint32_t width, uint32_t height) = 0;
    virtual void generateMipmaps(vk::CommandBuffer commandBuffer, vk::Image& image, vk::Format imageFormat, uint32_t texWidth, uint32_t texHeight, uint32_t mipLevels) = 0;
};

namespace Platform {
class Window;
}

class IPlatform {
public:
    virtual ~IPlatform() = default;

    virtual void initImGui() = 0;
    virtual void tick(float deltaTime) = 0;
    virtual void shutdownImGui() = 0;

    virtual std::shared_ptr<Platform::Window>& getMainWindow() = 0;
};

class IEngine {
public:
    virtual ~IEngine() = default;

    virtual void exit() = 0;
    virtual void exit(const std::string& reason) = 0;

    virtual IRenderer* getRenderer() const = 0;
    virtual IPlatform* getPlatform() const = 0;
};

} // namespace dirk
