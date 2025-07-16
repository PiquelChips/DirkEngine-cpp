#pragma once

#include <GLFW/glfw3.h>
#include <chrono>
#include <memory>
#include <string>

#include "core/globals.hpp"
#include "render/renderer.hpp"
#include "render/renderer_types.hpp"
#include "resources/resource_manager.hpp"

DECLARE_LOG_CATEGORY_EXTERN(LogEngine)

namespace dirk {

struct DirkEngineCreateInfo {
    ResourceManagerCreateInfo resourceManagerInfo;
    RendererCreateInfo rendererInfo;
};

class DirkEngine {

public:
    DirkEngine(DirkEngineCreateInfo& createInfo);

    int main();
    void exit();
    void exit(const std::string& reason);

    bool isRequestingExit() const noexcept { return requestingExit; }

    RendererCreateInfo RENDERER_INFO{ "DirkEngine", 800, 600, Vulkan, this };

    Renderer* getRenderer() const noexcept;
    ResourceManager* getResourceManager() const noexcept;

private:
    int init();
    void tick(float deltaTime);

    void cleanup();

private:
    std::unique_ptr<Renderer> renderer = nullptr;
    std::unique_ptr<ResourceManager> resourceManager = nullptr;

    bool requestingExit = false;

private:
    float captureDeltaTime();

    std::chrono::high_resolution_clock::time_point lastTick;
};

} // namespace dirk
