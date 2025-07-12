#include "engine/dirkengine.hpp"
#include "core/globals.hpp"
#include "render/renderer.hpp"

#include <GLFW/glfw3.h>
#include <chrono>
#include <cstdlib>
#include <cstring>

DEFINE_LOG_CATEGORY(LogEngine)

namespace dirk {

DirkEngine* gEngine = nullptr;

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

void DirkEngine::exit(const std::string& reason) {
    requestingExit = true;
    DIRK_LOG(LogEngine, INFO, "engine exit has been requested with reason: " << reason);
}

int DirkEngine::init() {
    int result = EXIT_SUCCESS;

    renderer = std::unique_ptr<Renderer>(createRenderer(RENDERER_INFO));
    check(renderer);
    result = renderer->init();
    if (result != EXIT_SUCCESS)
        return result;

    // TODO: init audio, network, input, ...

    DIRK_LOG(LogEngine, INFO, "engine initialization successful");

    return result;
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

} // namespace dirk
