#pragma once

#include <GLFW/glfw3.h>
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "actor.hpp"
#include "core/globals.hpp"
#include "render/renderer.hpp"
#include "render/renderer_types.hpp"
#include "resources/resource_manager.hpp"

namespace dirk {

DECLARE_LOG_CATEGORY_EXTERN(LogEngine)

struct DirkEngineCreateInfo {
    ResourceManagerCreateInfo resourceManagerInfo;
    RendererCreateInfo rendererInfo;
};

class DirkEngine {

public:
    DirkEngine(DirkEngineCreateInfo& createInfo);

    int run() { return main(); }

    void exit();
    void exit(const std::string& reason);

    bool isRequestingExit() const noexcept { return requestingExit; }

    Renderer* getRenderer() const noexcept;
    ResourceManager* getResourceManager() const noexcept;

    // managing actors
    // TODO: create a world class to manage actors
public:
    Actor* spawnActor(ActorSpawnInfo& spawnInfo);
    void destroyActor(Actor* actor);

private:
    std::unordered_map<std::string, Actor*> actors;

private:
    int main();
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
