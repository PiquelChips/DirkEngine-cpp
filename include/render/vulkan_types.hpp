#pragma once

#include "core/globals.hpp"

#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <sstream>

namespace dirk {

struct Transform {
    glm::vec3 location;
    glm::vec3 rotation;
    glm::vec3 scale;

    glm::mat4 getMatrix();
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }

    friend std::stringstream& operator<<(std::stringstream& stream, Vertex vertex) {
        stream << "position: " << vertex.pos.x << vertex.pos.y << vertex.pos.z << "\n"
               << "texCoord: " << vertex.texCoord.x << vertex.texCoord.y;

        return stream;
    }

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

struct ModelViewProjection {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

/**
 * DirkEngine's representation of a shader
 */
struct Shader {
    const std::string& name;

    std::size_t size;
    std::vector<char> shader;
};

/**
 * DirkEngine's representation of a texture
 */
struct Texture {
    std::uint32_t width, height;
    std::size_t size;
    std::vector<unsigned char> texture;
};

/**
 * DirkEngine's representation of a 3D model.
 *
 * This is essentially an object created by the resource manager from a glTF file.
 */
struct Model {
    const std::string& name;

    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    Texture texture;
};

class DirkEngine;

enum RenderApi {
    Vulkan,
};

struct RendererProperties {
    std::string applicationName;
    uint32_t windowWidth, windowHeight;
    RenderApi api;
    DirkEngine* engine;
};

struct RendererCreateInfo {
    std::string applicationName;
    uint32_t windowWidth, windowHeight;
    RenderApi api;
    DirkEngine* engine;

    operator RendererProperties();
};

struct RendererFeatures {
    bool anisotropy = false;
    int msaaSamples = 1;

    bool isComplete() { return anisotropy && msaaSamples > 1; }

    int getScore() {
        if (isComplete())
            return 1000;

        int score = 0;

        if (anisotropy)
            score += 10;

        score += msaaSamples;

        return score;
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

} // namespace dirk

namespace std {
template <>
struct hash<dirk::Vertex> {
    size_t operator()(dirk::Vertex const& vertex) const {
        return ((hash<glm::vec3>()(vertex.pos) ^
                 (hash<glm::vec3>()(vertex.color) << 1)) >>
                1) ^
               (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
};
} // namespace std
