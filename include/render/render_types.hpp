#pragma once

#include "glm/glm.hpp"

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
};

struct ModelViewProjection {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};
