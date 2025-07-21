#include "core/globals.hpp"

#include "engine/actor.hpp"

namespace dirk {

/**
 * The physical representation of a shape in the world.
 * Must be owned by an actor.
 * Will be overriden by various renderer subclasses for API
 * specific functionnality.
 */
class Mesh {
public:
    inline Actor* getOwningActor() { return owningActor; }

    inline virtual void setVisibity(bool inVisible) { isVisible = inVisible; }
    inline virtual bool visible() { return isVisible; }

    const Transform& getTransform() { return getOwningActor()->getTransform(); }
    const glm::mat4& getTransformMatrix() { return getOwningActor()->getTransformMatrix(); }

private:
    Actor* owningActor;

    bool isVisible = false;
};

} // namespace dirk
