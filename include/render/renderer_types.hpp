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
    bool anisotropy = false;
    int msaaSamples = 1;

    bool isComplete() { return anisotropy && msaaSamples > 1; }

    int getScore() {
        if (isComplete())
            return 1000;

        int score = 0;

        if (anisotropy)
            score += 10;

        score += msaaSamples;

        return score;
    }
};
