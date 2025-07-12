#pragma once

#include <cstdint>
#include <string>

#include "renderer_types.hpp"

class DirkEngine;

/**
 *  The base class for a renderer
 *  This class should not be implemented directly
 */
class Renderer {
public:
    virtual int init() = 0;
    virtual void draw(float deltaTime) = 0;
    virtual void cleanup() = 0;
};

Renderer* createRenderer(RendererCreateInfo& createInfo);
