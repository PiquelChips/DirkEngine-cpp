#pragma once

#include <cstdint>
#include <string>

class DirkEngine;

enum RenderApi {
    VulkanApi,
};

struct RendererInfo {
    std::string applicationName;
    RenderApi api;
};

struct RendererCreateInfo {
    std::string applicationName;
    uint32_t windowWdith, windowHeight;
    RenderApi api;
    DirkEngine* engine;
    // TODO: ResourceManager* resourceManaer;
};
