#pragma once

#include "core/globals.hpp"
#include "render/render.hpp"
#include "render/render_types.hpp"
#include "render/vulkan/vulkan_types.hpp"

#include "GLFW/glfw3.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"

#include <cstdint>
#include <tuple>
#include <vector>

// fix this somehow to allow ppl to disable them even in debug builds
#ifdef DEBUG_BUILD
#ifndef DISABLE_VALIDATION_LAYERS
#define ENABLE_VALIDATION_LAYERS
#endif
#endif

DECLARE_LOG_CATEGORY_EXTERN(LogVulkan)
DECLARE_LOG_CATEGORY_EXTERN(LogVulkanValidation)

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
    vk::Instance createVulkanInstance();
    std::vector<const char*> getRequiredInstanceExtensions();
    bool checkRequiredInstanceExtensions(std::vector<const char*>& extensions);

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
    std::vector<SwapChainImage> createSwapChainImages(std::vector<vk::Image>& images);

    void recreateSwapChain();

    vk::RenderPass createRenderPass();
    vk::CommandPool createCommandPool();
    vk::Pipeline createGraphicsPipeline();
    vk::DescriptorSetLayout createDescriptorSetLayout();
    vk::DescriptorPool createDescriptorPool();

    // texture, depth buffer & MSAA
    vk::Image createTextureImage();
    vk::Image createDepthResources();
    vk::Image createColorResources();

    // vertices & indices
    vk::Buffer createVertexBuffer();
    vk::Buffer createIndexBuffer();
    bool loadModel();

    // TODO: these could be combined and return a struct
    std::tuple<vk::Image, vk::DeviceMemory> createImage(uint32_t width, uint32_t height, vk::SampleCountFlagBits numSamples, uint32_t mipLevels, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties); // TODO: many params could have default values
    vk::ImageView createImageView(vk::Image& image, vk::Format format, vk::ImageAspectFlags imageAspect, uint32_t mipLevels);                                                                                                                               // TODO: many params could use default values
    vk::Sampler createTextureSampler();

    // TODO: next 3 § of functions should be static in a utils class
    std::tuple<vk::Buffer, vk::DeviceMemory> createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
    static vk::SampleCountFlagBits getMaxUsableSampleCount(vk::PhysicalDevice physicalDevice);

    vk::CommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(vk::CommandBuffer& commandBuffer);

    // TODO: all should take a buffer ref to only need to use one buffer for the entire op if chained
    void transitionImageLayout(const vk::Image& image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels);
    void copyBuffer(vk::Buffer& srcBuffer, vk::Buffer& dstBuffer, vk::DeviceSize size);
    void copyBufferToImage(vk::Buffer& buffer, vk::Image& image, uint32_t width, uint32_t height);
    void generateMipmaps(vk::Image& image, vk::Format imageFormat, uint32_t texWidth, uint32_t texHeight, uint32_t mipLevels);

    std::vector<InFlightImage> createInFlightImages(const int imageCount);

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

    // TODO: make into a struct
    uint32_t mipLevels;
    vk::Image textureImage;
    // TODO: vk::DeviceMemory textureImageMemory;
    vk::ImageView textureImageView;
    vk::Sampler textureSampler;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    vk::Buffer vertexBuffer;
    // TODO: vk::DeviceMemory vertexBufferMemory;
    vk::Buffer indexBuffer;
    // TODO: vk::DeviceMemory indexBufferMemory;

    // TODO: make into a struct
    vk::Image depthImage;
    // TDOO vk::DeviceMemory depthImageMemory;
    vk::ImageView depthImageView;
    vk::Format depthFormat;

    // TODO: make into a struct
    vk::Image colorImage;
    // TODO: vk::DeviceMemory colorImageMemory;
    vk::ImageView colorImageView;

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
    // misc variables used by the renderer

    RendererConfig rendererConfig;
    const std::vector<const char*> deviceExtensions = { vk::KHRSwapchainExtensionName };
    const int MAX_FRAMES_IN_FLIGHT = 2;                                // dont make this too high or CPU will go faster than GPU, causing latency
    vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1; // TODO: in renderer features struct
};
