#include "window.hpp"

#include <memory>

// representation of a viewport of the engine (essentially a renderer output)
namespace dirk {

class Viewport {
public:
    Viewport();
    ~Viewport();

    void render();
    glm::vec2 getSize() const;

private:
    std::shared_ptr<Platform::Window> window;
};

} // namespace dirk
