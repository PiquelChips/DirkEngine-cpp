#include "engine/world.hpp"

namespace dirk {

World::World(const std::vector<ActorCreateInfo>& actors) {
    for (auto createInfo : actors) {
        spawnActor(createInfo);
    }
}

World::~World() {
    actors.clear();
}

void World::tick(float deltaTime) {
    for (auto pair : actors) {
        pair.second->tick(deltaTime);
    }
}

std::shared_ptr<Actor> World::spawnActor(ActorCreateInfo spawnInfo) {
    DIRK_LOG(LogEngine, INFO, "spawning actor " << spawnInfo.name);
    std::shared_ptr<Actor> actor = std::make_shared<Actor>(spawnInfo, this);
    actors[actor->getName()] = actor;
    return actor;
}

void World::destroyActor(Actor* actor) {
    actors.erase(actor->getName());
}

} // namespace dirk
