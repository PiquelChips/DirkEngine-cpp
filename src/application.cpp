#include "dirkengine.h"

dirk::DirkEngineCreateInfo dirk::getEngineCreateInfo(int argc, char** argv) {
    return dirk::DirkEngineCreateInfo{
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
}
