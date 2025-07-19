#include "engine/actor.hpp"
#include "engine/dirkengine.hpp"

namespace dirk {

Actor::Actor(ActorSpawnInfo& spawnInfo) : name(spawnInfo.name), engine(spawnInfo.engine) {}

void Actor::destroy() { engine->destroyActor(this); }

void Actor::deinitialize() {}

} // namespace dirk
