#pragma once

#include "core/globals.hpp"
#include "renderer_types.hpp"

class DirkEngine;

DECLARE_LOG_CATEGORY_EXTERN(LogRenderer);

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
