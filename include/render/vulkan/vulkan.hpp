#pragma once

#include "GLFW/glfw3.h"
#include "logger.hpp"
#include "render/render.hpp"
#include <cstdint>
#include <optional>
#include <vector>

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
    VkQueue graphicsQueue;
    VkQueue presentQueue;
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct SwapChainImage {
    VkImageView imageView;
    VkFramebuffer frameBuffer;
};

struct InFlightImage {
    VkCommandBuffer commandBuffer;
    // syncing
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;
};

/**
 * The vulkan implementation of the renderer
 */
class VulkanRenderer : public Renderer {

public:
    VulkanRenderer(RendererConfig rendererConfig, Logger* logger);

    int init() override;
    void tick(DirkEngine* engine) override;
    int draw() override;
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
    int getDeviceSuitability(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

    void createLogicalDevice();

    // swap chain
    std::vector<VkImage> createSwapChain();
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void createSwapChainImages(std::vector<VkImage> images);

    void createRenderPass();
    void createCommandPool();
    void createGraphicsPipeline();

    void createInFlightImages(const int imageCount);

    const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#ifdef ENABLE_VALIDATION_LAYERS
public:
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

private:
    bool checkValidationLayerSupport();
    void setupDebugMessenger();

    std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
    VkDebugUtilsMessengerEXT debugMessenger;
#endif

private:
    // vulkan stuff

    // base required objects
    GLFWwindow* window = nullptr;
    VkInstance instance = nullptr;
    Queues queues;
    VkSurfaceKHR surface;

    // devices
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    // swap chain
    VkSwapchainKHR swapChain;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    // rendering
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkCommandPool commandPool;

    std::vector<SwapChainImage> swapChainImages;
    std::vector<InFlightImage> inFlightImages;

    uint32_t currentFrame = 1;

private:
    // drawing, should be removed and improved later on
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void drawFrame();

    // shader utilities
    VkShaderModule loadShaderModule(const std::string& shaderName);

private:
    // dont make this too high or CPU will go faster than GPU, causing latency
    const int MAX_FRAMES_IN_FLIGHT = 2;
};
