#include "dirkengine.h"

dirk::DirkEngineCreateInfo dirk::getEngineCreateInfo(int argc, char** argv) {
    return dirk::DirkEngineCreateInfo{
        .rendererInfo = dirk::RendererCreateInfo{
            .applicationName = "DirkEngine",
            .windowWidth = 1200,
            .windowHeight = 800,
        },
        .actorCreateInfos{
            ActorCreateInfo{
                .name = "Shrek",
                .modelName = "Shrek",
                .transform = Transform{
                    .location = glm::vec3(0.f, 0.f, 0.f),
                    .rotation = glm::vec3(0.f),
                    .scale = glm::vec3(1.f),
                },
            },
            ActorCreateInfo{
                .name = "Duck",
                .modelName = "Duck",
                .transform = Transform{
                    .location = glm::vec3(100.f, 0.f, 0.f),
                    .rotation = glm::vec3(0.f),
                    .scale = glm::vec3(1.f),
                },
            },
            ActorCreateInfo{
                .name = "Duck's friend",
                .modelName = "Duck",
                .transform = Transform{
                    .location = glm::vec3(-100.f, 0.f, 0.f),
                    .rotation = glm::vec3(-90.f, 0.f, 0.f),
                    .scale = glm::vec3(1.f),
                },
            },
            ActorCreateInfo{
                .name = "Viking Room",
                .modelName = "viking_room",
                .transform = Transform{
                    .location = glm::vec3(0.f, -200.f, -200.f),
                    .rotation = glm::vec3(-135.f, 0.f, 0.f),
                    .scale = glm::vec3(600.f),
                },
            },
        },
    };
}
