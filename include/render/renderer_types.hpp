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

struct RendererFeatures {
    bool anisotropy;

    bool isComplete() { return anisotropy; }

    int getScore() {
        if (isComplete())
            return 1000;

        int score = 0;

        if (anisotropy)
            score += 10;

        return score;
    }
};
