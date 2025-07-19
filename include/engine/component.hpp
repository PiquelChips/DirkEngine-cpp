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

    const std::string& getName() const { return name; }
    const glm::mat4& getTransformMatrix() const { return transformMatrix; }

    inline const Transform& getTransform() const { return transform; }
    inline void setTransform(const Transform& inTransform);

    inline const glm::vec3& getLocation() const { return transform.location; }
    inline void setLocation(const glm::vec3& inLocation);

    inline const glm::vec3& getRotation() const { return transform.rotation; }
    inline void setRotation(const glm::vec3& inRotation);

    inline const glm::vec3& getScale() const { return transform.scale; }
    inline void setScale(const glm::vec3& inScale);

private:
    void updateTransformMatrix();
    Transform transform;
    glm::mat4 transformMatrix{ 1.f };

protected:
    Actor* getOwningActor() const { return owningActor; }

private:
    Actor* owningActor;
    const std::string& name;
};

} // namespace dirk
