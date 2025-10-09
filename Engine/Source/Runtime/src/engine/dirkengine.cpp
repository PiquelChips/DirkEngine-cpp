#include "engine/dirkengine.hpp"

#include "core/logging.hpp"
#include "engine/world.hpp"
#include "render/renderer.hpp"
#include "render/viewport.hpp"
#include "render/window.hpp"
#include "vulkan/vulkan_structs.hpp"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace dirk {

DEFINE_LOG_CATEGORY(LogEngine)

DirkEngine::DirkEngine(const DirkEngineCreateInfo& createInfo) {
    renderer = std::make_unique<Renderer>(createInfo.rendererInfo);
    world = std::make_unique<World>(createInfo.actorCreateInfos);

    auto viewportId = renderer->createViewport(ViewportCreateInfo{});
    auto windowId = createWindow(WindowCreateInfo{
        .title = createInfo.appName,
        .width = 1200,
        .height = 800,
    });
    windows[windowId]->addViewport(viewportId, vk::Rect2D{ { 0, 0 }, { 1, 1 } }, 0);

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

WindowId DirkEngine::createWindow(const WindowCreateInfo& createInfo) {
    // TODO: properly assign window IDs
    WindowId id = windows.size();
    windows[id] = std::make_unique<Window>();
    return id;
}

void DirkEngine::destroyWindow(WindowId id) {
    windows.erase(id);
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
