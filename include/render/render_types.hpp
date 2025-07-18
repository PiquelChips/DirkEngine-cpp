#pragma once

#include "core/globals.hpp"

#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"

#include <cstdint>
#include <functional>
#include <vector>

namespace dirk {

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};

struct ModelViewProjection {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

/**
 * DirkEngine's representation of a texture
 */
struct Texture {
    uint32_t width, height, size;
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
        return (
                   (
                       hash<glm::vec3>()(vertex.pos) ^
                       (hash<glm::vec3>()(vertex.color) << 1)) >>
                   1) ^
               (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
};
} // namespace std
