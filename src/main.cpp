#include "dirkengine.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>

class DirkGame : public dirk::GameInstance {
    int initialize() override {
        return EXIT_SUCCESS;
    }
    void begin() override {}
    void deinitialize() override {}
};

int main() {
    try {
        dirk::DirkEngineCreateInfo engineInfo{
            .resourceManagerInfo = dirk::ResourceManagerCreateInfo{
                .resourcePath = RESOURCE_PATH,
                .shaderPath = SHADER_PATH,
            },
            .rendererInfo = dirk::RendererCreateInfo{
                .applicationName = "DirkEngine",
                .windowWidth = 800,
                .windowHeight = 600,
                .api = dirk::Vulkan,
            },
            .gameInstance = std::make_unique<DirkGame>(),
        };

        auto engine = std::make_unique<dirk::DirkEngine>(engineInfo);
        return engine->run();
    } catch (std::exception e) {
        std::cerr << e.what();
        return EXIT_FAILURE;
    }
}
