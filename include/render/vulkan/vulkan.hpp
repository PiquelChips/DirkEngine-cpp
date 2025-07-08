#pragma once

#include "core/globals.hpp"
#include "render/render.hpp"

#include "GLFW/glfw3.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"

#include <cstdint>
#include <optional>
#include <tuple>

// fix this somehow to allow ppl to disable them even in debug builds
#ifdef DEBUG_BUILD
#ifndef DISABLE_VALIDATION_LAYERS
#define ENABLE_VALIDATION_LAYERS
#endif
#endif

DECLARE_LOG_CATEGORY_EXTERN(LogVulkan)
DECLARE_LOG_CATEGORY_EXTERN(LogVulkanValidation)

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct Queues {
    vk::Queue graphicsQueue;
    vk::Queue presentQueue;
};

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

struct SwapChainImage {
    vk::ImageView imageView;
    vk::Framebuffer frameBuffer;

    operator bool() const { return imageView && frameBuffer; }
};

struct InFlightImage {
    vk::CommandBuffer commandBuffer;
    // syncing
    vk::Semaphore imageAvailableSemaphore;
    vk::Semaphore renderFinishedSemaphore;
    vk::Fence inFlightFence;
    // ubo for the mvp
    vk::Buffer uniformBuffer;
    vk::DeviceMemory uniformBufferMemory;
    void* uniformBufferMapped;
    // descriptor set for the ubo
    vk::DescriptorSet descriptorSet;

    operator bool() const { return commandBuffer &&
                                   imageAvailableSemaphore && renderFinishedSemaphore && inFlightFence &&
                                   uniformBuffer && uniformBufferMapped && uniformBufferMemory; }
};

/**
 * The vulkan implementation of the renderer
 */
class VulkanRenderer : public Renderer {

public:
    VulkanRenderer(RendererConfig rendererConfig);

    int init() override;
    void draw(float deltaTime) override;
    void cleanup() override;

private:
    RendererConfig rendererConfig;

private:
    vk::Instance createVulkanInstance();
    std::vector<const char*> getRequiredInstanceExtensions();
    bool checkRequiredInstanceExtensions(std::vector<const char*> extensions);

    vk::SurfaceKHR createSurface();

    // selecting the physical device
    vk::PhysicalDevice getPhysicalDevice();
    int getDeviceSuitability(vk::PhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device);
    bool checkDeviceExtensionSupport(vk::PhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device);

    vk::Device createLogicalDevice();
    Queues createQueues();

    // swap chain
    std::vector<vk::Image> createSwapChain(vk::SwapchainKHR oldSwapChain);
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
    std::vector<SwapChainImage> createSwapChainImages(std::vector<vk::Image> images);

    void recreateSwapChain();

    vk::RenderPass createRenderPass();
    vk::CommandPool createCommandPool();
    vk::Pipeline createGraphicsPipeline();
    vk::DescriptorSetLayout createDescriptorSetLayout();
    vk::DescriptorPool createDescriptorPool();

    vk::Buffer createVertexBuffer();
    vk::Buffer createIndexBuffer();
    std::tuple<vk::Buffer, vk::DeviceMemory> createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    void copyBuffer(vk::Buffer& srcBuffer, vk::Buffer& dstBuffer, vk::DeviceSize size);

    std::vector<InFlightImage> createInFlightImages(const int imageCount);

    const std::vector<const char*> deviceExtensions = { vk::KHRSwapchainExtensionName };

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

    static void frameBufferResizeCallback(GLFWwindow* window, int width, int height);

private:
    // vulkan stuff

    // base required objects
    GLFWwindow* window = nullptr;
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
    vk::DescriptorSetLayout descriptorSetLayout;
    vk::DescriptorPool descriptorPool;

    vk::Buffer vertexBuffer;
    vk::Buffer indexBuffer;

    std::vector<SwapChainImage> swapChainImages;
    std::vector<InFlightImage> inFlightImages;

    uint32_t currentFrame = 0;
    bool framebufferResized = false;

private:
    // drawing, should be removed and improved later on
    void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex);
    void drawFrame();
    void updateMVP(float deltaTime);

    // shader utilities
    vk::ShaderModule loadShaderModule(const std::string& shaderName);

private:
    // dont make this too high or CPU will go faster than GPU, causing latency
    const int MAX_FRAMES_IN_FLIGHT = 2;
};
