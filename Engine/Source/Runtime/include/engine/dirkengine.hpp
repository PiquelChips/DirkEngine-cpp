#pragma once

#include <chrono>
#include <memory>
#include <vector>

#include "actor.hpp"
#include "core/globals.hpp"
#include "render/vulkan_types.hpp"
#include "render/window.hpp"

namespace dirk {

DECLARE_LOG_CATEGORY_EXTERN(LogEngine)

class Renderer;
class World;
class Camera;

struct DirkEngineCreateInfo {
    const std::string& appName;
    RendererCreateInfo rendererInfo;
    std::vector<ActorCreateInfo> actorCreateInfos;
};

class DirkEngine {

public:
    DirkEngine(const DirkEngineCreateInfo& createInfo);
    ~DirkEngine();

    void exit();
    void exit(const std::string& reason);

    bool isRequestingExit() const noexcept { return requestingExit; }

    WindowId createWindow(const WindowCreateInfo& createInfo);
    void destroyWindow(WindowId id);
    std::unique_ptr<Window>& getWindow(WindowId id) { return windows[id]; }

private:
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<World> world;

    std::unordered_map<WindowId, std::unique_ptr<Window>> windows;

private:
    void tick(float deltaTime);
    float captureDeltaTime();

    std::chrono::high_resolution_clock::time_point lastTick;
    bool requestingExit = false;
};

} // namespace dirk
