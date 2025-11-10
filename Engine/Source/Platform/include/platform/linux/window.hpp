#ifdef PLATFORM_LINUX

#pragma once

#include "platform/platform.hpp"
#include "platform/window.hpp"
#include "wayland-client-core.h"

namespace dirk::Platform::Linux {

class LinuxWindow : public PlatformWindowImpl {
public:
    LinuxWindow();
    ~LinuxWindow();

    void* getNativeHandle();

    vk::SurfaceKHR createVulkanSurface(vk::Instance instance);

    vk::Extent2D getSize();
    void setSize(vk::Extent2D inSize);
    vk::Extent2D getFramebufferSize();

    glm::vec2 getPosition();
    void setPosition(const glm::vec2 inPosition);

    std::string_view getTitle();
    void setTitle(std::string_view inTitle);

    bool isFocused();
    bool isMinimized();
};

std::vector<const char*> getRequiredExtensions();

} // namespace dirk::Platform::Linux

#endif
