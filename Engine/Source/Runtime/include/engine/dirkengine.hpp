#pragma once

#include <chrono>
#include <memory>
#include <vector>

#include "actor.hpp"
#include "common.hpp"
#include "render/vulkan_types.hpp"
#include "render/window.hpp"

namespace dirk {

DECLARE_LOG_CATEGORY_EXTERN(LogEngine)

class Renderer;
class World;
class Camera;

struct DirkEngineCreateInfo {
    std::string_view appName;
    std::vector<ActorCreateInfo> actorCreateInfos;
};

class DirkEngine {

public:
    DirkEngine(const DirkEngineCreateInfo& createInfo);
    ~DirkEngine();

    void exit();
    void exit(const std::string& reason);

    bool isRequestingExit() const noexcept { return requestingExit; }

    std::shared_ptr<Window>& createWindow(const WindowCreateInfo& createInfo);
    void destroyWindow(std::shared_ptr<Window>& window);
    std::vector<std::shared_ptr<Window>>& getWindows() { return windows; }

    Renderer* getRenderer() { return renderer.get(); }

private:
    std::unique_ptr<Renderer> renderer;
    std::shared_ptr<World> world;

    std::vector<std::shared_ptr<Window>> windows;

private:
    void tick(float deltaTime);
    float captureDeltaTime();

    std::chrono::high_resolution_clock::time_point lastTick;
    bool requestingExit = false;
};

} // namespace dirk
