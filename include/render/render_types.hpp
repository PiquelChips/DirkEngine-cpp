#pragma once

#include "core/globals.hpp"

#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"

#include <functional>
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
