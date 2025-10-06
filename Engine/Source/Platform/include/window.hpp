#include "keys.hpp"

#include "glm/glm.hpp"
#include "vulkan/vulkan.hpp"

#include <cstdint>
#include <string>

#pragma once

namespace dirk::Platform {

struct WindowProperties {
    std::string applicationName;
    std::uint32_t windowWidth, windowHeight;
};

struct WindowCreateInfo {
    const std::string& title;
    uint32_t width, height;

    operator WindowProperties();
};

// representation of a system window
class Window {
public:
    Window(const WindowCreateInfo& createInfo);
    ~Window();

    // Basic window operations
    bool shouldClose() const;
    void pollEvents();
    glm::vec2 getSize() const;
    vk::SurfaceKHR createSruface(vk::Instance instance);
};

} // namespace dirk::Platform
