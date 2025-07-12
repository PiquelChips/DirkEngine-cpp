#pragma once

#include <cstdint>
#include <string>

enum RenderApi {
    VulkanApi,
};

struct RendererConfig {
    uint32_t width;
    uint32_t height;
    std::string name;
};

struct RendererCreateInfo {
    std::string applicationName;
    RenderApi api;
};
