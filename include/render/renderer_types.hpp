#pragma once

#include <cstdint>
#include <string>

namespace dirk {

class DirkEngine;

enum RenderApi {
    Vulkan,
};

struct RendererProperties {
    std::string applicationName;
    uint32_t windowWidth, windowHeight;
    RenderApi api;
    DirkEngine* engine;
};

struct RendererCreateInfo {
    std::string applicationName;
    uint32_t windowWidth, windowHeight;
    RenderApi api;
    DirkEngine* engine;

    operator RendererProperties();
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

} // namespace dirk
