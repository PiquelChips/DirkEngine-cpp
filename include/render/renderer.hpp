#pragma once

#include "core/globals.hpp"
#include "renderer_types.hpp"

namespace dirk {

DECLARE_LOG_CATEGORY_EXTERN(LogRenderer);

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

    RendererProperties& getProperties() noexcept;
    RendererFeatures& getFeatures() noexcept;

protected:
    RendererProperties properties;
    RendererFeatures features;

    DirkEngine* getEngine() { return getProperties().engine; };
};

Renderer* createRenderer(RendererCreateInfo& createInfo);

} // namespace dirk
