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
    // this constructor is used internally by the engine
    // please use the createComponent function
    Component(ComponentCreateInfo& createInfo);

    // begin Component interface
    virtual void initialize() = 0;
    virtual void tick(float deltaTime) = 0;
    virtual void deinitialize() = 0;
    // end Component interface

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
    /**
     * This is stored as a raw pointer as the component will be destroyed
     * before the owning actor
     */
    Actor* owningActor;
    const std::string& name;
};

/**
 * A raw pointer is returned as the lifetime of an actors components
 * is managed by the actor itself. Thus, the component will be destroyed
 * properly on actor shutdown.
 */
template <class T>
Component* createComponent(ComponentCreateInfo& createInfo);

} // namespace dirk
