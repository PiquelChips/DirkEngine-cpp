#include "window.hpp"

#include <memory>

#pragma once

// representation of a viewport of the engine (essentially a renderer output)
namespace dirk {

class Viewport {
public:
    Viewport();
    ~Viewport();

    void render();
    glm::vec2 getSize() const;

    // Input
    bool isKeyDown(Input::Key key);
    bool isMouseButtonDown(Input::MouseButton button);
    glm::vec2 getMousePosition();
    void setCursorMode(Input::CursorMode mode);

private:
    std::shared_ptr<Platform::Window> window;
};

} // namespace dirk
