#pragma once

// GLM
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

class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual std::vector<SwapChainImage> createSwapChain(const SwapChainCreateInfo& createInfo) = 0;

    virtual RendererResources getResources() = 0;
    virtual const RendererProperties& getProperties() = 0;
    virtual const DeviceFeatures getDeviceFeatures() = 0;
};

class IPlatform {
public:
    virtual void initImGui(const RendererResources& resources) = 0;
    virtual void tick(float deltaTime) = 0;
    virtual void shutdownImGui() = 0;
};

class IEngine {
public:
    virtual void exit() = 0;
    virtual void exit(const std::string& reason) = 0;

    virtual IRenderer* getRenderer() const = 0;
    virtual IPlatform* getPlatform() const = 0;
};

} // namespace dirk
