#include "engine/dirkengine.hpp"

#include "core/logging.hpp"
#include "glm/trigonometric.hpp"
#include "render/camera.hpp"
#include "render/renderer.hpp"

#include <GLFW/glfw3.h>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <utility>

namespace dirk {

DEFINE_LOG_CATEGORY(LogEngine)

DirkEngine::DirkEngine(DirkEngineCreateInfo& createInfo) {
    engine = this;

    renderer = std::make_shared<Renderer>(createInfo.rendererInfo);
    if (renderer->init() != EXIT_SUCCESS) {
        DIRK_LOG(LogEngine, FATAL, "unable to initialize renderer");
        return;
    }
    camera = std::make_shared<Camera>(glm::vec3(0.f, 400.f, 400.f), glm::vec3(0.f, -1.f, -1.f), glm::radians(45.f), .1f, 100000.f);

    lastTick = std::chrono::high_resolution_clock::now();
    for (auto actorCreateInfo : createInfo.actorCreateInfos) {
        spawnActor(actorCreateInfo);
    }

    while (true) {
        float deltaTime = captureDeltaTime();

        if (isRequestingExit())
            break;

        glfwPollEvents();

        tick(deltaTime);
    }
}

DirkEngine::~DirkEngine() {
    DIRK_LOG(LogEngine, INFO, "exiting");
    for (auto pair : actors) {
        pair.second->destroy();
    }
}

void DirkEngine::exit() {
    requestingExit = true;
}

void DirkEngine::exit(const std::string& reason) {
    DIRK_LOG(LogEngine, INFO, "engine exit has been requested with reason: " << reason);
    this->exit();
}

std::shared_ptr<Actor> DirkEngine::spawnActor(ActorCreateInfo spawnInfo) {
    DIRK_LOG(LogEngine, INFO, "spawning actor " << spawnInfo.name);
    std::shared_ptr<Actor> actor = std::make_shared<Actor>(spawnInfo);
    actors[actor->getName()] = actor;
    return actor;
}

void DirkEngine::destroyActor(Actor* actor) {
    actors.erase(actor->getName());
}

void DirkEngine::tick(float deltaTime) {
    Camera::get()->tick(deltaTime);
    for (auto pair : actors) {
        pair.second->tick(deltaTime);
    }

    Renderer::get()->draw(deltaTime);
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
