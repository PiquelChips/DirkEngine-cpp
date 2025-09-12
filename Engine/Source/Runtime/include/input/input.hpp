#include "input/keys.hpp"

#include "glm/glm.hpp"

namespace dirk {

class Input {
public:
    static bool isKeyDown(Key key);
    static bool isMouseButtonDown(MouseButton button);

    static glm::vec2 getMousePosition();

    static void setCursorMode(CursorMode mode);
};

} // namespace dirk
