#pragma once

#include <GLFW/glfw3.h>
#include <chrono>
#include <memory>
#include <string>

#include "core/globals.hpp"
#include "render/renderer.hpp"
#include "render/renderer_types.hpp"

namespace dirk {

DECLARE_LOG_CATEGORY_EXTERN(LogEngine)

class DirkEngine {

public:
    int main();
    void exit(const std::string& reason);

    bool isRequestingExit() const noexcept { return requestingExit; }

    RendererCreateInfo RENDERER_INFO{ "DirkEngine", 800, 600, VulkanApi, this };

private:
    int init();
    void tick(float deltaTime);

    void cleanup();

private:
    std::unique_ptr<Renderer> renderer = nullptr;

    bool requestingExit = false;

private:
    float captureDeltaTime();

    std::chrono::high_resolution_clock::time_point lastTick;
};

} // namespace dirk
