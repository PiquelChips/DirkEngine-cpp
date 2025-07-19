#include "render/render_types.hpp"

namespace dirk {

class Actor;

struct ComponentCreateInfo {
    const std::string& name;
    Actor* owningActor;
};

/**
 * A component can be added to any actor for added & reusable functionality
 */
class Component {

public:
    Component(ComponentCreateInfo& createInfo);

    void deinitialize();

    inline const Transform& getTransform() const { return transform; }
    inline void setTransform(const Transform& transform);

    inline const glm::vec3& getLocation() const { return transform.location; }
    inline void setLocation(const glm::vec3& inLocation);

    inline const glm::vec3& getRotation() const { return transform.rotation; }
    inline void setRotation(const glm::vec3& inRotation);

    inline const glm::vec3& getScale() const { return transform.scale; }
    inline void setScale(const glm::vec3& inScale);

private:
    Transform transform;

protected:
    Actor* getOwningActor() const { return owningActor; }

private:
    Actor* owningActor;
};

} // namespace dirk
