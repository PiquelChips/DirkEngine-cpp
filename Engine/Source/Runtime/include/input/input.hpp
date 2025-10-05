#include "input/keys.hpp"

#include "glm/glm.hpp"

namespace dirk::Input {

bool isKeyDown(Key key);
bool isMouseButtonDown(MouseButton button);

glm::vec2 getMousePosition();

void setCursorMode(CursorMode mode);

} // namespace dirk::Input
