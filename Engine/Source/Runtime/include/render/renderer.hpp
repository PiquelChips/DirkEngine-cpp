#pragma once

#include "core.hpp"
#include "engine/dirkengine.hpp"
#include "render/viewport.hpp"

#include "imgui.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"

#include <cstdint>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>

#ifdef DIRK_DEVELOPMENT_BUILD
#ifndef DISABLE_VALIDATION_LAYERS
#define ENABLE_VALIDATION_LAYERS
#endif
#endif

namespace dirk {

DECLARE_LOG_CATEGORY_EXTERN(LogRenderer)
DECLARE_LOG_CATEGORY_EXTERN(LogVulkan)
DECLARE_LOG_CATEGORY_EXTERN(LogVulkanValidation)

#define MAX_DESCRIPTOR_SET_COUNT 20 // incrementally increase as scenes get bigger

constexpr glm::vec3 RIGHT_DIRECTION{ 1.f, 0.f, 0.f };
constexpr glm::vec3 UP_DIRECTION{ 0.f, 1.f, 0.f }; // Y-up
constexpr glm::vec3 FORWARD_DIRECTION{ 0.f, 0.f, 1.f };

struct ImGuiViewportRendererData {
    vk::SwapchainKHR swapchain;
    vk::SurfaceKHR surface;
    vk::CommandBuffer commandBuffer;

    // renderer settings
    vk::Extent2D swapChainExtent;
    vk::SurfaceFormatKHR surfaceFormat;
    vk::PresentModeKHR presentMode;

    std::vector<SwapchainImage> swapChainImages;
    std::vector<std::tuple<vk::Semaphore, vk::Semaphore>> semaphores;

    // state
    std::uint32_t imageIndex = 0;
    std::uint32_t semaphoreIndex = 0;
    bool swapchainNeedsRecreation = true; // swap chain will thus be created on first render call

    static constexpr vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
};

/**
 * The vulkan implementation of the renderer
 */
class Renderer : public IRenderer {
public:
    Renderer();
    ~Renderer();

    void init(vk::SurfaceKHR surface);
    void ImGui_init(vk::SurfaceKHR surface);
    void render();
    void ImGui_shutdown();

    void ImGui_beginFrame();
    void ImGui_render();

    std::vector<SwapchainImage> createSwapChain(const SwapChainCreateInfo& createInfo);

    vk::ShaderModule loadShaderModule(const std::string& shaderName);
    vk::Semaphore createSemaphore();
    vk::CommandBuffer createCommandBuffer();
    vk::DescriptorSet createDescriptorSets(vk::Buffer uniformBuffer, vk::Sampler sampler, vk::ImageView imageView, vk::ImageLayout layout);

    inline RendererResources getResources() { return RendererResources{
        .instance = instance,
        .physicalDevice = physicalDevice,
        .device = device,
        .queues = queues,
        .commandPool = commandPool,
        .descriptorSetLayout = descriptorSetLayout,
    }; }
    inline const RendererProperties& getProperties() { return properties; }
    inline const DeviceFeatures getDeviceFeatures() { return getDeviceFeatures(physicalDevice); }

    // swap chain
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
    vk::Extent2D chooseSwapExtent(vk::Extent2D windowSize, const vk::SurfaceCapabilitiesKHR& capabilities);

private:
    ImGuiViewportRendererData* mainViewportData = nullptr; // TODO: remove for custom backend

private:
    bool checkRequiredInstanceExtensions(std::vector<const char*>& extensions);

    int getDeviceSuitability(vk::PhysicalDevice device, vk::SurfaceKHR surface);
    bool checkDeviceExtensionSupport(vk::PhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface);

    void ImGui_createWindow(ImGuiViewport* viewport);
    void ImGui_renderWindow(ImGuiViewport* viewport);
    void ImGui_destroyWindow(ImGuiViewport* viewport);

#ifdef ENABLE_VALIDATION_LAYERS
private:
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        vk::DebugUtilsMessageTypeFlagsEXT messageType,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    bool checkValidationLayerSupport();

    vk::DebugUtilsMessengerEXT debugMessenger;
#endif

    // vulkan resources
    vk::Instance instance;

    vk::PhysicalDevice physicalDevice;
    vk::Device device;
    vk::CommandPool commandPool;
    Queues queues;

    vk::DescriptorSetLayout descriptorSetLayout;
    vk::DescriptorPool descriptorPool;

    vk::Fence inFlightFence;

    // misc variables used by the renderer
    RendererProperties properties;

    static constexpr std::array<const char*, 1> deviceExtensions = { vk::KHRSwapchainExtensionName };
    static constexpr std::array<const char*, 1> validationLayers = { "VK_LAYER_KHRONOS_validation" };

public:
    // some utility functions
    ImageMemoryView createImageMemoryView(CreateImageMemoryViewInfo& createInfo);

    std::tuple<vk::Image, vk::DeviceMemory> createImage(
        uint32_t width, uint32_t height, vk::Format format,
        vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties,
        vk::SampleCountFlagBits numSamples = vk::SampleCountFlagBits::e1,
        uint32_t mipLevels = 1);

    vk::ImageView createImageView(vk::Image& image, vk::Format format, vk::ImageAspectFlags imageAspect = vk::ImageAspectFlagBits::eColor, uint32_t mipLevels = 1);

    std::tuple<vk::Buffer, vk::DeviceMemory> createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);

    vk::CommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(vk::CommandBuffer& commandBuffer, vk::Queue queue);

    void transitionImageLayout(vk::CommandBuffer commandBuffer, const vk::Image& image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels = 1);
    void copyBufferToImage(vk::CommandBuffer commandBuffer, vk::Buffer& buffer, vk::Image& image, uint32_t width, uint32_t height, uint32_t offsetX = 0, uint32_t offsetY = 0);
    void generateMipmaps(vk::CommandBuffer commandBuffer, vk::Image& image, vk::Format imageFormat, uint32_t texWidth, uint32_t texHeight, uint32_t mipLevels);

public:
    // static utils
    static uint32_t findMemoryType(vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    static vk::Format findSupportedFormat(vk::PhysicalDevice physicalDevice, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
    static vk::SampleCountFlagBits getMaxUsableSampleCount(vk::PhysicalDevice physicalDevice);
    static bool hasStencilComponent(vk::Format format);
    static DeviceFeatures getDeviceFeatures(vk::PhysicalDevice physicalDevice);
};

} // namespace dirk
