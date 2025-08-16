#pragma once

#include <GLFW/glfw3.h>
#include <chrono>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "actor.hpp"
#include "render/vulkan_types.hpp"

namespace dirk {

DECLARE_LOG_CATEGORY_EXTERN(LogEngine)

class Renderer;
class Camera;

struct DirkEngineCreateInfo {
    RendererCreateInfo rendererInfo;
    std::vector<ActorCreateInfo> actorCreateInfos;
};

DirkEngineCreateInfo getEngineCreateInfo(int argc, char** argv);

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
    static std::shared_ptr<Camera> getCamera() { return get()->camera; }

private:
    inline static DirkEngine* engine;
    std::shared_ptr<Renderer> renderer;
    std::shared_ptr<Camera> camera;

    // TODO: create a world class to manage actors
public:
    std::unordered_map<std::string_view, std::shared_ptr<Actor>>& getActors() { return actors; }
    std::shared_ptr<Actor> spawnActor(ActorCreateInfo spawnInfo);
    void destroyActor(Actor* actor);

private:
    std::unordered_map<std::string_view, std::shared_ptr<Actor>> actors;
    // END

private:
    void tick(float deltaTime);
    float captureDeltaTime();

    std::chrono::high_resolution_clock::time_point lastTick;
    bool requestingExit = false;
};

} // namespace dirk
