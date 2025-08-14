#pragma once

#include <GLFW/glfw3.h>
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "actor.hpp"
#include "core/globals.hpp"
#include "render/vulkan.hpp"
#include "render/vulkan_types.hpp"
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

    Renderer* getRenderer() const noexcept { return renderer.get(); }
    ResourceManager* getResourceManager() const noexcept { return resourceManager.get(); }

    // managing actors
    // TODO: create a world class to manage actors
public:
    template <class T>
    std::shared_ptr<T> spawnActor(ActorSpawnInfo& spawnInfo);
    // for internal use only
    void unregisterActor(Actor* actor);

private:
    std::unordered_map<std::string, std::shared_ptr<Actor>> actors;

private:
    int main();
    int init();
    void tick(float deltaTime);

    void cleanup();

private:
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<ResourceManager> resourceManager;

    bool requestingExit = false;

private:
    float captureDeltaTime();

    std::chrono::high_resolution_clock::time_point lastTick;
};

} // namespace dirk
