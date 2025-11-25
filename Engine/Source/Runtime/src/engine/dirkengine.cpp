#include "engine/dirkengine.hpp"

#include "common.hpp"
#include "engine/world.hpp"
#include "platform/window.hpp"
#include "render/renderer.hpp"
#include "render/viewport.hpp"
#include "vulkan/vulkan_structs.hpp"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace dirk {

DEFINE_LOG_CATEGORY(LogEngine)

IEngine* gEngine;

DirkEngine::DirkEngine(const DirkEngineCreateInfo& createInfo) {
    gEngine = this;

    renderer = std::make_unique<Renderer>();
    // platform needs renderer init
    platform = std::make_unique<Platform::Platform>(createInfo.platformCreateInfo);
    // we need renderer var to be initialized for this function to run
    renderer->init();
    world = std::make_shared<World>(createInfo.actorCreateInfos);

    auto viewport = renderer->createViewport(ViewportCreateInfo{
        .name = "Hello World!",
        .size = vk::Extent2D(500, 500),
        .world = world,
    });

    lastTick = std::chrono::high_resolution_clock::now();

    while (true) {
        float deltaTime = captureDeltaTime();

        if (isRequestingExit())
            break;

        tick(deltaTime);
    }
}

DirkEngine::~DirkEngine() {
    DIRK_LOG(LogEngine, INFO, "exiting");
}

void DirkEngine::exit() {
    requestingExit = true;
}

void DirkEngine::exit(const std::string& reason) {
    DIRK_LOG(LogEngine, INFO, "engine exit has been requested with reason: " << reason);
    this->exit();
}

void DirkEngine::tick(float deltaTime) {
    platform->tick(deltaTime);
    // in case we fail platform event polling
    if (isRequestingExit())
        return;

    world->tick(deltaTime);

    renderer->render();
}

float DirkEngine::captureDeltaTime() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto duration = currentTime - lastTick;
    float deltaTime = std::chrono::duration<float>(duration).count();

    lastTick = currentTime;

    // DIRK_LOG(LogDirkEngine, INFO) << "delta time: " << deltaTime;
    // DIRK_LOG(LogDirkEngine, INFO) << "fps: " << 1.0 / deltaTime;

    return deltaTime;
}

} // namespace dirk
