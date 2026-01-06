#pragma once

#include "Events/EventManager.hpp"
#include "actor.hpp"
#include "core.hpp"
#include "platform/platform.hpp"

#include <chrono>
#include <memory>
#include <vector>

namespace dirk {

DECLARE_LOG_CATEGORY_EXTERN(LogEngine)

class Renderer;
class World;
class Camera;

struct DirkEngineCreateInfo {
    std::string_view appName;
    Platform::PlatformCreateInfo platformCreateInfo;
    std::vector<ActorCreateInfo> actorCreateInfos;
};

class DirkEngine : public IEngine {

public:
    DirkEngine(const DirkEngineCreateInfo& createInfo);
    ~DirkEngine();

    void exit();
    void exit(const std::string& reason);

    bool isRequestingExit() const noexcept { return requestingExit; }

    IRenderer* getRenderer() const { return (IRenderer*) renderer.get(); }
    EventManager* getEventManager() const { return eventManager.get(); }

private:
    std::unique_ptr<EventManager> eventManager;
    std::unique_ptr<Platform::Platform> platform;
    std::unique_ptr<Renderer> renderer;
    std::shared_ptr<World> world;

private:
    bool tick(float deltaTime);
    bool render(float deltaTime);
    float captureDeltaTime();
    void renderImGui(float deltaTime);

    std::chrono::high_resolution_clock::time_point lastTick;
    bool requestingExit = false;

    // ImGui UI state
    bool showDemoWindow, showStyleEditor;
};

} // namespace dirk
