#ifdef PLATFORM_LINUX

#pragma once

#include "platform/linux/linux.hpp"
#include "platform/platform.hpp"
#include "platform/window.hpp"

#include "vulkan/vulkan_handles.hpp"

namespace dirk::Platform::Linux {

class LinuxWindowImpl : public PlatformWindowImpl {
public:
    LinuxWindowImpl(const WindowCreateInfo& createInfo, LinuxPlatformImpl& platformImpl);
    ~LinuxWindowImpl();

    vk::SurfaceKHR getVulkanSurface(vk::Instance instance) override;
    void* getPlatformHandle() override { return wlSurface; }

    vk::Extent2D getSize() override { return size; }
    void setSize(vk::Extent2D inSize) override;

    std::string_view getTitle() override { return title; }
    void setTitle(std::string_view inTitle) override;

    // TODO: xdg-activation protocol
    bool isFocused() override { return focused; }
    void focus(bool inFocused) override;

    // TODO: xdg-decoration protocol
    bool isDecorated() override { return decorated; }
    void setDecorated(bool inDecorated) override;

    // TODO: throw DIRK_LOG error as not implemented in wayland
    glm::vec2 getPosition() override;
    void setPosition(const glm::vec2 inPosition) override;

private:
    // window properties
    vk::Extent2D size;
    std::string_view title;
    bool focused;
    bool minimized;
    bool maximized;
    bool decorated;

    LinuxPlatformImpl& linuxPlatform;

    wl_surface* wlSurface;
    xdg_surface* xdgSurface;
    xdg_toplevel* xdgToplevel;

    vk::SurfaceKHR vkSurface;
};

} // namespace dirk::Platform::Linux

#endif
