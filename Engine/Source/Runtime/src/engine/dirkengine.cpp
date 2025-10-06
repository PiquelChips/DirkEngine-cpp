#include "engine/dirkengine.hpp"

#include "core/logging.hpp"
#include "engine/world.hpp"
#include "render/renderer.hpp"
#include "render/window.hpp"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace dirk {

DEFINE_LOG_CATEGORY(LogEngine)

DirkEngine::DirkEngine(const DirkEngineCreateInfo& createInfo) {

    renderer = std::make_shared<Renderer>(createInfo.rendererInfo);
    // TODO: remove & do init in constructor
    if (renderer->init() != EXIT_SUCCESS) {
        DIRK_LOG(LogEngine, FATAL, "unable to initialize renderer");
        return;
    }
    world = std::make_shared<World>(createInfo.actorCreateInfos);

    lastTick = std::chrono::high_resolution_clock::now();

    while (true) {
        float deltaTime = captureDeltaTime();

        if (isRequestingExit())
            break;

        for (auto& [id, window] : windows) {
            window->processPlatformEvents();
        }

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

std::shared_ptr<Window> DirkEngine::createWindow(const WindowCreateInfo& createInfo) {
    auto window = std::make_shared<Window>(createInfo);
    // TODO: properly assign window IDs
    windows[windows.size()] = window;
    return window;
}

void DirkEngine::tick(float deltaTime) {
    world->tick(deltaTime);

    renderer->renderFrame();
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
