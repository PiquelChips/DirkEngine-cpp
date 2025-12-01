#include "engine/world.hpp"

namespace dirk {

World::World(const std::vector<ActorCreateInfo>& actors) {
    for (auto spawnInfo : actors) {
        spawnActor(spawnInfo);
    }
}

World::~World() {
    actors.clear();
}

void World::tick(float deltaTime) {
    for (auto& pair : actors) {
        pair.second->tick(deltaTime);
    }
}

void World::spawnActor(const ActorCreateInfo& spawnInfo) {
    DIRK_LOG(LogEngine, INFO, "spawning actor {}", spawnInfo.name);
    auto actor = std::make_unique<Actor>(spawnInfo, *this);
    actors[actor->getName()] = std::move(actor);
}

void World::destroyActor(Actor* actor) {
    actors.erase(actor->getName());
}

} // namespace dirk
