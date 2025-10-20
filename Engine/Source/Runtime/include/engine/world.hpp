#pragma once

#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "engine/actor.hpp"
#include "engine/dirkengine.hpp"

namespace dirk {

class World {

public:
    World(const std::vector<ActorCreateInfo>& actors);
    ~World();

    void tick(float deltaTime);

    std::unordered_map<std::string_view, std::shared_ptr<Actor>>& getActors() { return actors; }
    std::shared_ptr<Actor>& spawnActor(const ActorCreateInfo& spawnInfo);
    void destroyActor(Actor* actor);

private:
    std::unordered_map<std::string_view, std::shared_ptr<Actor>> actors;
};

} // namespace dirk
