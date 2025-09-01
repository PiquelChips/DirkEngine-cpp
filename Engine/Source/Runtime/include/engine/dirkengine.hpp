#pragma once

#include <chrono>
#include <memory>
#include <vector>

#include "GLFW/glfw3.h"

#include "actor.hpp"
#include "render/vulkan_types.hpp"

namespace dirk {

DECLARE_LOG_CATEGORY_EXTERN(LogEngine)

class Renderer;
class World;
class Camera;

struct DirkEngineCreateInfo {
    RendererCreateInfo rendererInfo;
    std::vector<ActorCreateInfo> actorCreateInfos;
};

class DirkEngine {

public:
    DirkEngine(DirkEngineCreateInfo& createInfo);
    ~DirkEngine();

    void exit();
    void exit(const std::string& reason);

    bool isRequestingExit() const noexcept { return requestingExit; }

public:
    static DirkEngine* get() { return engine; }
    static std::shared_ptr<Renderer> getRenderer() { return get()->renderer; }
    static std::shared_ptr<World> getWorld() { return get()->world; }
    static std::shared_ptr<Camera> getCamera() { return get()->camera; }

private:
    inline static DirkEngine* engine;
    std::shared_ptr<Renderer> renderer;
    std::shared_ptr<World> world;
    std::shared_ptr<Camera> camera;

private:
    void tick(float deltaTime);
    float captureDeltaTime();

    std::chrono::high_resolution_clock::time_point lastTick;
    bool requestingExit = false;
};

} // namespace dirk
