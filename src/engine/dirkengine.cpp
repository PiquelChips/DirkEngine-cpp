#include "engine/dirkengine.hpp"
#include "render/vulkan/vulkan.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cstdlib>
#include <cstring>

DirkEngine::DirkEngine() {
    logger = std::make_unique<Logger>();
    assert(logger);
}

int DirkEngine::main() {
    int result = EXIT_SUCCESS;
    result = init();

    if (result != EXIT_SUCCESS)
        return result;

    while (true) {
        // mainly check if GLFW close event is called
        // place before isRequestingExit as this could call DirkEngine::exit()
        renderer->tick(this);

        if (isRequestingExit())
            break;

        glfwPollEvents();
        tick();
    }

    getLogger()->Get(INFO) << "exiting";
    cleanup();

    return result;
}

void DirkEngine::exit(const std::string& reason) {
    requestingExit = true;
    getLogger()->Get(INFO) << "engine exit has been requested with reason: " << reason;
}

int DirkEngine::init() {
    int result = EXIT_SUCCESS;

    // for now we only use Vulkan
    renderer = std::make_unique<VulkanRenderer>(RENDER_CONFIG, logger.get());
    result = renderer->init();
    if (result != EXIT_SUCCESS)
        return result;

    // TODO: init audio
    // TODO: init network
    // TODO: init input

    return result;
}

void DirkEngine::tick() {
    renderer->draw();
}

void DirkEngine::cleanup() {
    renderer->cleanup();
}
