#pragma once

#include "common.hpp"

#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"

#include <cstdint>
#include <optional>

namespace dirk {

class DirkEngine;

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

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

} // namespace dirk
