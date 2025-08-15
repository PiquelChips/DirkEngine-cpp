#pragma once

#include <GLFW/glfw3.h>
#include <chrono>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "actor.hpp"
#include "core/globals.hpp"
#include "render/render_types.hpp"
#include "render/renderer.hpp"
#include "render/vulkan_types.hpp"
#include "resources/resource_manager.hpp"

namespace dirk {

DECLARE_LOG_CATEGORY_EXTERN(LogEngine)

struct DirkEngineCreateInfo {
    ResourceManagerCreateInfo resourceManagerInfo;
    RendererCreateInfo rendererInfo;
    const std::vector<ActorCreateInfo>& actorCreateInfos;
};

class DirkEngine {

public:
    DirkEngine(DirkEngineCreateInfo& createInfo);

    void exit();
    void exit(const std::string& reason);

    bool isRequestingExit() const noexcept { return requestingExit; }

    Renderer* getRenderer() const noexcept { return renderer.get(); }
    ResourceManager* getResourceManager() const noexcept { return resourceManager.get(); }

    // TODO: create a world class to manage actors
public:
    std::unordered_map<std::string_view, std::shared_ptr<Actor>>& getActors() { return actors; }
    std::shared_ptr<Actor> spawnActor(ActorCreateInfo spawnInfo);
    void destroyActor(Actor* actor);

private:
    std::unordered_map<std::string_view, std::shared_ptr<Actor>> actors;
    // END

private:
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

DirkEngineCreateInfo getEngineCreateInfo(int argc, char** argv);

} // namespace dirk
