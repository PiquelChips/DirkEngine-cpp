#ifdef PLATFORM_LINUX

#pragma once

#include "platform/linux/linux.hpp"
#include "platform/window.hpp"

#include "vulkan/vulkan_handles.hpp"

namespace dirk::Platform::Linux {

class LinuxWindowImpl : public PlatformWindowImpl {
public:
    LinuxWindowImpl(const WindowCreateInfo& createInfo, LinuxPlatformImpl& platformImpl);
    ~LinuxWindowImpl();

    void show() override;
    void hide() override;

    vk::SurfaceKHR getVulkanSurface(vk::Instance instance) override;
    void* getPlatformHandle() override { return wlSurface; }

    vk::Extent2D getSize() override { return size; }
    void setSize(vk::Extent2D inSize) override;

    std::string_view getTitle() override { return title; }
    void setTitle(std::string_view inTitle) override;

    // TODO: xdg-activation protocol
    bool isFocused() override;
    void focus() override;

    // TODO: xdg-decoration protocol
    bool isDecorated() override { return decorated; }
    void setDecorated(bool inDecorated) override;

    glm::vec2 getPosition() override;
    void setPosition(const glm::vec2 inPosition) override;

private:
    // window properties
    vk::Extent2D size;
    std::string_view title;
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
