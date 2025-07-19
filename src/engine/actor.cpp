#include "engine/actor.hpp"
#include "engine/dirkengine.hpp"

namespace dirk {

void Actor::destroy() { engine->destroyActor(this); }

} // namespace dirk
