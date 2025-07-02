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
    virtual int init() { return 0; };
    virtual void tick(DirkEngine* engine) {};
    virtual int draw() { return 0; };
    virtual void cleanup() {};
};
