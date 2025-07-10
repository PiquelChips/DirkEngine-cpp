#pragma once

#include "glm/glm.hpp"

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
};

struct ModelViewProjection {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};
