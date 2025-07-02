#pragma once

#include <cstdint>
#include <string>

class DirkEngine;

struct RendererConfig {
    uint32_t width;
    uint32_t height;
    std::string name;
};

/**
 *  The base class for a renderer
 *  This class should not be implemented directly
 */
class Renderer {
public:
    virtual int init() = 0;
    virtual void tick(DirkEngine* engine) = 0;
    virtual int draw() = 0;
    virtual void cleanup() = 0;
};
