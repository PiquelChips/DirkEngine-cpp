#include "engine/dirkengine.hpp"
#include "render/vulkan_types.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>

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
            },
        };

        auto engine = std::make_unique<dirk::DirkEngine>(engineInfo);
        return engine->run();
    } catch (std::exception e) {
        std::cerr << e.what();
        return EXIT_FAILURE;
    }
}
