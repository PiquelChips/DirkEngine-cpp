#pragma once

#include <chrono>
#include <memory>
#include <vector>

#include "actor.hpp"
#include "common.hpp"
#include "platform/platform.hpp"

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

private:
    std::unique_ptr<Platform::Platform> platform;
    std::unique_ptr<Renderer> renderer;
    std::shared_ptr<World> world;

private:
    bool tick(float deltaTime);
    float captureDeltaTime();
    void renderImGui();

    std::chrono::high_resolution_clock::time_point lastTick;
    bool requestingExit = false;
};

} // namespace dirk
