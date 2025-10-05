#pragma once

#include <chrono>
#include <memory>
#include <vector>

#include "actor.hpp"
#include "core/globals.hpp"
#include "render/vulkan_types.hpp"
#include "window.hpp"

#include "GLFW/glfw3.h"

namespace dirk {

DECLARE_LOG_CATEGORY_EXTERN(LogEngine)

class Renderer;
class World;
class Camera;

struct DirkEngineCreateInfo {
    RendererCreateInfo rendererInfo;
    Platform::WindowCreateInfo windowInfo;
    std::vector<ActorCreateInfo> actorCreateInfos;
};

class DirkEngine {

public:
    DirkEngine(const DirkEngineCreateInfo& createInfo);
    ~DirkEngine();

    void exit();
    void exit(const std::string& reason);

    bool isRequestingExit() const noexcept { return requestingExit; }

public:
    static DirkEngine* get() { return engine; }
    static std::shared_ptr<Renderer> getRenderer() { return get()->renderer; }
    static std::shared_ptr<World> getWorld() { return get()->world; }
    static std::shared_ptr<Camera> getCamera() { return get()->camera; }
    static std::shared_ptr<Platform::Window> getWindow() { return get()->window; }

private:
    inline static DirkEngine* engine;
    std::shared_ptr<Renderer> renderer;
    std::shared_ptr<World> world;
    std::shared_ptr<Camera> camera;
    std::shared_ptr<Platform::Window> window;

private:
    void tick(float deltaTime);
    float captureDeltaTime();

    std::chrono::high_resolution_clock::time_point lastTick;
    bool requestingExit = false;
};

} // namespace dirk
