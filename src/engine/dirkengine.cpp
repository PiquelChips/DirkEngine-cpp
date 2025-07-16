#include "engine/dirkengine.hpp"
#include "core/globals.hpp"
#include "render/renderer.hpp"
#include "resources/resource_manager.hpp"

#include <GLFW/glfw3.h>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <memory>

DEFINE_LOG_CATEGORY(LogEngine)

namespace dirk {

DirkEngine::DirkEngine(DirkEngineCreateInfo& createInfo) {
    // make sure to populate the engine fields
    createInfo.resourceManagerInfo.engine = this;
    createInfo.rendererInfo.engine = this;

    resourceManager = std::make_unique<ResourceManager>(createInfo.resourceManagerInfo);
    renderer = std::unique_ptr<Renderer>(createRenderer(createInfo.rendererInfo));
    check(renderer);
}

int DirkEngine::main() {
    int result = EXIT_SUCCESS;
    result = init();

    if (result != EXIT_SUCCESS)
        return result;

    lastTick = std::chrono::high_resolution_clock::now();

    while (true) {
        float deltaTime = captureDeltaTime();

        if (isRequestingExit())
            break;

        glfwPollEvents();

        tick(deltaTime);
    }

    DIRK_LOG(LogEngine, INFO, "exiting");
    cleanup();

    return result;
}

void DirkEngine::exit() {
    requestingExit = true;
}

void DirkEngine::exit(const std::string& reason) {
    DIRK_LOG(LogEngine, INFO, "engine exit has been requested with reason: " << reason);
    this->exit();
}

int DirkEngine::init() {
    if (renderer->init() != EXIT_SUCCESS)
        return EXIT_FAILURE;

    // TODO: init audio, network, input, ...

    DIRK_LOG(LogEngine, INFO, "engine initialization successful");

    return EXIT_SUCCESS;
}

void DirkEngine::tick(float deltaTime) {
    renderer->draw(deltaTime);
}

void DirkEngine::cleanup() {
    renderer->cleanup();
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

Renderer* DirkEngine::getRenderer() const noexcept { return renderer.get(); }
ResourceManager* DirkEngine::getResourceManager() const noexcept { return resourceManager.get(); }

} // namespace dirk
