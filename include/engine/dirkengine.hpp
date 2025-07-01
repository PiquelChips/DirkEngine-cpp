#pragma once

#include <GLFW/glfw3.h>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "logger.hpp"
#include "vulkan/vk_platform.h"
#include "vulkan/vulkan_core.h"

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

class DirkEngine {

public:
    DirkEngine(Logger* logger);

    bool init();
    void start();
    void exit();

    bool isRequestingExit() const noexcept;

    Logger* getLogger() const noexcept;

private:
    void initWindow();
    void initVulkan();

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
    void createSwapChain();
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void createImageViews();

    void createRenderPass();
    void createGraphicsPipeline();

    void tick();
    void cleanup();

public:
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;
    const std::string NAME = "Dirk Engine";

    const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

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

    std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
    VkDebugUtilsMessengerEXT debugMessenger;
#endif

private:
    GLFWwindow* window = nullptr;
    VkInstance instance = nullptr;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    VkSurfaceKHR surface;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    std::vector<VkImageView> swapChainImageViews;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    Queues queues;

    Logger* logger = nullptr;

    bool initSuccessful = false;
    bool requestingExit = false;
};
