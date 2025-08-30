#include "input/input.hpp"

#include "input/keys.hpp"
#include "render/renderer.hpp"

#include "GLFW/glfw3.h"
#include "glm/fwd.hpp"

namespace dirk {

bool Input::isKeyDown(Key key) {
    GLFWwindow* window = Renderer::get()->getWindow();
    int state = glfwGetKey(window, (int) key);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool Input::isMouseButtonDown(MouseButton button) {
    GLFWwindow* window = Renderer::get()->getWindow();
    int state = glfwGetMouseButton(window, (int) button);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

glm::vec2 Input::getMousePosition() {
    GLFWwindow* window = Renderer::get()->getWindow();

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    return glm::vec2(x, y);
}

void Input::setCursorMode(CursorMode mode) {
    GLFWwindow* window = Renderer::get()->getWindow();
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL + (int) mode);
}

} // namespace dirk
