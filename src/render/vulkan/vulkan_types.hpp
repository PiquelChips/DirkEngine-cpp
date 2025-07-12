#pragma once

#include "render/render_types.hpp"

#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"

#include <cstdint>
#include <optional>

struct VulkanVertex : Vertex {
    static vk::VertexInputBindingDescription getBindingDescription() {
        return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
    }

    static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
            vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord))
        };
    }
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
    vk::Fence inFlightFence;
    // ubo for the mvp
    vk::Buffer uniformBuffer;
    vk::DeviceMemory uniformBufferMemory;
    void* uniformBufferMapped;
    // descriptor set for the ubo
    vk::DescriptorSet descriptorSet;

    operator bool() const { return commandBuffer && inFlightFence && uniformBuffer && uniformBufferMapped && uniformBufferMemory; }
};

struct ImageMemoryView {
    vk::Image image;
    vk::DeviceMemory memory;
    vk::ImageView view;

    operator bool() const { return image && memory && view; }
};

struct CreateImageMemoryViewInfo {
    vk::Device device;
    vk::PhysicalDevice physicalDevice;

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
