#include "engine/dirkengine.hpp"

dirk::DirkEngineCreateInfo engineCreateInfo{
    .appName = "DirkEngine",
    .platformCreateInfo = dirk::Platform::PlatformCreateInfo{
        .appName = "DirkEngine",
    },
    .actorCreateInfos{
        dirk::ActorCreateInfo{
            .name = "Shrek",
            .modelName = "Shrek",
            .transform = dirk::Transform{
                .location = glm::vec3(0.f, 0.f, 0.f),
                .rotation = glm::vec3(0.f),
                .scale = glm::vec3(1.f),
            },
        },
        dirk::ActorCreateInfo{
            .name = "Duck",
            .modelName = "Duck",
            .transform = dirk::Transform{
                .location = glm::vec3(100.f, 0.f, 0.f),
                .rotation = glm::vec3(0.f),
                .scale = glm::vec3(1.f),
            },
        },
        dirk::ActorCreateInfo{
            .name = "Duck's friend",
            .modelName = "Duck",
            .transform = dirk::Transform{
                .location = glm::vec3(-100.f, 0.f, 0.f),
                .rotation = glm::vec3(-90.f, 0.f, 0.f),
                .scale = glm::vec3(1.f),
            },
        },
        dirk::ActorCreateInfo{
            .name = "Viking Room",
            .modelName = "viking_room",
            .transform = dirk::Transform{
                .location = glm::vec3(0.f, -200.f, -200.f),
                .rotation = glm::vec3(-135.f, 0.f, 0.f),
                .scale = glm::vec3(600.f),
            },
        },
    },
};

int main(int, char**) {
    dirk::DirkEngine engine = dirk::DirkEngine(engineCreateInfo);
}
