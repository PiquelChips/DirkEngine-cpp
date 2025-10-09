#pragma once

#include "core/globals.hpp"
#include "engine/dirkengine.hpp"
#include "render/viewport.hpp"
#include "render/window.hpp"
#include "render_types.hpp"
#include "vulkan_types.hpp"

#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"

#include <cstdint>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>

#ifdef DEBUG_BUILD
#ifndef DISABLE_VALIDATION_LAYERS
#define ENABLE_VALIDATION_LAYERS
#endif
#endif

namespace dirk {

DECLARE_LOG_CATEGORY_EXTERN(LogRenderer)
DECLARE_LOG_CATEGORY_EXTERN(LogVulkan)
DECLARE_LOG_CATEGORY_EXTERN(LogVulkanValidation)

/**
 * The vulkan implementation of the renderer
 */
class Renderer {

public:
    Renderer(const RendererCreateInfo& createInfo);
    ~Renderer();

    void renderFrame();

    const RendererFeatures& getFeatures() const noexcept { return features; }

    void registerWindow(Window* window);
    void unregisterWindow(Window* window);

    ViewportId createViewport(const ViewportCreateInfo& createInfo);
    void destroyViewport(ViewportId id);
    std::shared_ptr<Viewport> getViewport(ViewportId id);

private:
    std::unordered_map<WindowId, std::shared_ptr<Window>> windows;
    std::unordered_map<ViewportId, std::shared_ptr<Viewport>> viewports;

private:
    vk::Instance createVulkanInstance();
    std::vector<const char*> getRequiredInstanceExtensions();
    bool checkRequiredInstanceExtensions(std::vector<const char*>& extensions);

    vk::SurfaceKHR createSurface();

    // selecting the physical device
    vk::PhysicalDevice selectPhysicalDevice();
    int getDeviceSuitability(vk::PhysicalDevice device);
    bool checkDeviceExtensionSupport(vk::PhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device);

    vk::Device createLogicalDevice();
    Queues createQueues();

    // swap chain
    std::vector<vk::Image> createSwapChain();
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
    std::vector<SwapChainImage> createSwapChainImages(std::vector<vk::Image>& images);

    void recreateSwapChain();

    vk::RenderPass createRenderPass();
    vk::CommandPool createCommandPool();
    vk::Pipeline createGraphicsPipeline();
    vk::DescriptorSetLayout createDescriptorSetLayout();
    vk::DescriptorPool createDescriptorPool();

    // improve overall image quality during rendering
    ImageMemoryView createDepthResources();
    ImageMemoryView createColorResources();

    std::vector<InFlightImage> createInFlightImages(const int imageCount);

public:
    vk::Device getLogicalDevice() { return device; }
    vk::PhysicalDevice getPhysicalDevice() { return physicalDevice; }
    Queues getQueues() { return queues; }
    vk::Extent2D getSwapChainExtent() { return swapChainExtent; }

    vk::DescriptorSet createDescriptorSets(vk::Buffer uniformBuffer, vk::Sampler sampler, vk::ImageView imageView, vk::ImageLayout layout);

#ifdef ENABLE_VALIDATION_LAYERS
private:
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        vk::DebugUtilsMessageTypeFlagsEXT messageType,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    bool checkValidationLayerSupport();
    vk::DebugUtilsMessengerEXT setupDebugMessenger();

    std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
    vk::DebugUtilsMessengerEXT debugMessenger;
#endif

private:
    // vulkan stuff

    // base required objects
    std::shared_ptr<Window> window;
    vk::Instance instance;

    Queues queues;
    vk::SurfaceKHR surface;

    // devices
    vk::PhysicalDevice physicalDevice;
    vk::Device device;

    // swap chain
    vk::SwapchainKHR swapChain;
    vk::Format swapChainImageFormat;
    vk::Extent2D swapChainExtent;

    // rendering
    vk::RenderPass renderPass;
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline graphicsPipeline;
    vk::CommandPool commandPool;

    ImageMemoryView depthImageMemoryView;
    vk::Format depthFormat;

    ImageMemoryView colorImageMemoryView;

    vk::DescriptorSetLayout descriptorSetLayout;
    vk::DescriptorPool descriptorPool;

    std::vector<SwapChainImage> swapChainImages;
    std::vector<InFlightImage> inFlightImages;
    std::vector<std::tuple<vk::Semaphore, vk::Semaphore>> semaphores;

    uint32_t currentFrame = 0;
    uint32_t currentSemaphore = 0;
    bool framebufferResized = false;

private:
    // drawing, should be removed and improved later on
    void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

private:
    // misc variables used by the renderer

    const std::vector<const char*> deviceExtensions = { vk::KHRSwapchainExtensionName };
    const int MAX_FRAMES_IN_FLIGHT = 2;      // dont make this too high or CPU will go faster than GPU, causing latency
    const int MAX_DESCRIPTOR_SET_COUNT = 20; // incrementally increase as scenes get bigger
    vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;

    RendererFeatures features;

public:
    // some static utility functions
    ImageMemoryView createImageMemoryView(CreateImageMemoryViewInfo& createInfo);

    std::tuple<vk::Image, vk::DeviceMemory> createImage(
        uint32_t width, uint32_t height, vk::Format format,
        vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties,
        vk::SampleCountFlagBits numSamples = vk::SampleCountFlagBits::e1,
        uint32_t mipLevels = 1);

    vk::ImageView createImageView(vk::Image& image, vk::Format format, vk::ImageAspectFlags imageAspect = vk::ImageAspectFlagBits::eColor, uint32_t mipLevels = 1);

    std::tuple<vk::Buffer, vk::DeviceMemory> createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
    static uint32_t findMemoryType(vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    static vk::Format findSupportedFormat(vk::PhysicalDevice physicalDevice, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
    static vk::SampleCountFlagBits getMaxUsableSampleCount(vk::PhysicalDevice physicalDevice);
    QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device);

    vk::CommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(vk::CommandBuffer& commandBuffer, vk::Queue queue);

    static void transitionImageLayout(vk::CommandBuffer commandBuffer, const vk::Image& image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels = 1);
    static void copyBuffer(vk::CommandBuffer commandBuffer, vk::Buffer& srcBuffer, vk::Buffer& dstBuffer, vk::DeviceSize size);
    static void copyBufferToImage(vk::CommandBuffer commandBuffer, vk::Buffer& buffer, vk::Image& image, uint32_t width, uint32_t height);
    void generateMipmaps(vk::CommandBuffer commandBuffer, vk::Image& image, vk::Format imageFormat, uint32_t texWidth, uint32_t texHeight, uint32_t mipLevels);

    static bool hasStencilComponent(vk::Format format);

    static RendererFeatures getRendererFeatures(vk::PhysicalDevice physicalDevice);

    vk::ShaderModule loadShaderModule(const std::string& shaderName);
};

} // namespace dirk
