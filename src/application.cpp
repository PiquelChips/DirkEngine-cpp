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
        .actorCreateInfos{
            ActorCreateInfo{
                .name = "Duck",
                .modelName = "Duck",
                .transform = Transform{
                    .location = glm::vec3(0.f),
                    .rotation = glm::vec3(0.f),
                    .scale = glm::vec3(1.f),
                },
            },
        }
    };
}
