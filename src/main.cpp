#include "core/globals.hpp"
#include "engine/dirkengine.hpp"
#include "render/renderer_types.hpp"
#include "resources/resource_manager.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>

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
        };

        auto engine = std::make_unique<dirk::DirkEngine>(engineInfo);
        return engine->main();
    } catch (std::exception e) {
        std::cerr << e.what();
        return EXIT_FAILURE;
    }
}
