#pragma once

#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"

#include "core/globals.hpp"

class VulkanUtils {
public:
    // TODO: these could be combined and return a struct
    static std::tuple<vk::Image, vk::DeviceMemory> createImage(vk::Device device, vk::PhysicalDevice physicalDevice, uint32_t width, uint32_t height, vk::SampleCountFlagBits numSamples, uint32_t mipLevels, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties); // TODO: many params could have default values
    static vk::ImageView createImageView(vk::Device device, vk::Image& image, vk::Format format, vk::ImageAspectFlags imageAspect, uint32_t mipLevels);                                                                                                                                                                  // TODO: many params could use default values

    static std::tuple<vk::Buffer, vk::DeviceMemory> createBuffer(vk::Device device, vk::PhysicalDevice physicalDevice, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
    static uint32_t findMemoryType(vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    static vk::Format findSupportedFormat(vk::PhysicalDevice physicalDevice, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
    static vk::SampleCountFlagBits getMaxUsableSampleCount(vk::PhysicalDevice physicalDevice);

    static vk::CommandBuffer beginSingleTimeCommands(vk::Device device, vk::CommandPool commandPool);
    static void endSingleTimeCommands(vk::CommandBuffer& commandBuffer, vk::Queue queue);

    // TODO: all should take a buffer ref to only need to use one buffer for the entire op if chained
    static void transitionImageLayout(vk::CommandBuffer commandBuffer, const vk::Image& image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels);
    static void copyBuffer(vk::CommandBuffer commandBuffer, vk::Buffer& srcBuffer, vk::Buffer& dstBuffer, vk::DeviceSize size);
    static void copyBufferToImage(vk::CommandBuffer commandBuffer, vk::Buffer& buffer, vk::Image& image, uint32_t width, uint32_t height);
    static void generateMipmaps(vk::CommandBuffer commandBuffer, vk::PhysicalDevice physicalDevice, vk::Image& image, vk::Format imageFormat, uint32_t texWidth, uint32_t texHeight, uint32_t mipLevels);

    static constexpr bool hasStencilComponent(vk::Format format);
};
