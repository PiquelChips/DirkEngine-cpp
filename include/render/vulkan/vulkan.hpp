#pragma once

#include "GLFW/glfw3.h"
#include "core/logger.hpp"
#include "render/render.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"

#include <cstdint>
#include <optional>

// fix this somehow to allow ppl to disable them even in debug builds
#ifdef DEBUG_BUILD
#define ENABLE_VALIDATION_LAYERS
#endif

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

    operator bool() const { return commandBuffer && imageAvailableSemaphore && renderFinishedSemaphore && inFlightFence; }
};

/**
 * The vulkan implementation of the renderer
 */
class VulkanRenderer : public Renderer {

public:
    VulkanRenderer(RendererConfig rendererConfig, Logger* logger);

    int init() override;
    void draw(float deltaTime) override;
    void cleanup() override;

private:
    Logger* getLogger() const noexcept { return logger; };
    Logger* logger = nullptr;

    RendererConfig rendererConfig;

public:
    int initWindow();
    int initVulkan();

    void createVulkanInstance();
    std::vector<const char*> getRequiredInstanceExtensions();

    void createSurface();

    // selecting the physical device
    void getPhysicalDevice();
    int getDeviceSuitability(vk::PhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device);
    bool checkDeviceExtensionSupport(vk::PhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device);

    void createLogicalDevice();

    // swap chain
    std::vector<vk::Image> createSwapChain();
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
    void createSwapChainImages(std::vector<vk::Image> images);

    void createRenderPass();
    void createCommandPool();
    void createGraphicsPipeline();

    void createInFlightImages(const int imageCount);

    const std::vector<const char*> deviceExtensions = { vk::KHRSwapchainExtensionName };

#ifdef ENABLE_VALIDATION_LAYERS
public:
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        vk::DebugUtilsMessageTypeFlagsEXT messageType,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

private:
    bool checkValidationLayerSupport();
    void setupDebugMessenger();

    std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
    vk::DebugUtilsMessengerEXT debugMessenger;
#endif

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

    std::vector<SwapChainImage> swapChainImages;
    std::vector<InFlightImage> inFlightImages;

    uint32_t currentFrame = 0;

private:
    // drawing, should be removed and improved later on
    void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex);
    void drawFrame();

    // shader utilities
    vk::ShaderModule loadShaderModule(const std::string& shaderName);

private:
    // dont make this too high or CPU will go faster than GPU, causing latency
    const int MAX_FRAMES_IN_FLIGHT = 2;
};
