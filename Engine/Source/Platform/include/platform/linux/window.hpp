#ifdef PLATFORM_LINUX

#pragma once

#include "platform/linux/linux.hpp"
#include "platform/window.hpp"

#include "vulkan/vulkan_handles.hpp"
#include "wayland-util.h"

#include <cstdint>

namespace dirk::Platform::Linux {

class LinuxWindowImpl : public PlatformWindowImpl {
public:
    LinuxWindowImpl(const WindowCreateInfo& createInfo, LinuxPlatformImpl& platformImpl);
    ~LinuxWindowImpl();

    void show() override;
    void hide() override;

    vk::SurfaceKHR getVulkanSurface(vk::Instance instance) override;
    void createVulkanSurface(VkInstance instance, VkSurfaceKHR* surface) override;
    void* getPlatformHandle() override { return wlSurface; }

    vk::Extent2D getSize() override { return size; }
    void setSize(vk::Extent2D inSize) override { size = inSize; }

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

    void setOwningWindow(Window& window) override { owningWindow = &window; }
    Window& getOwningWindow() override {
        check(owningWindow);
        return *owningWindow;
    }

private:
    static void xdg_ToplevelConfigure(void* data, xdg_toplevel* toplevel, int32_t width, int32_t height, wl_array* states);
    static void xdg_ToplevelClose(void* data, xdg_toplevel* toplevel);

private:
    // window properties
    vk::Extent2D size;
    std::string_view title;
    bool decorated;

    LinuxPlatformImpl& linuxPlatform;

    wl_surface* wlSurface = nullptr;
    xdg_surface* xdgSurface = nullptr;
    xdg_toplevel* xdgToplevel = nullptr;

    vk::SurfaceKHR vkSurface;

    Window* owningWindow;
};

} // namespace dirk::Platform::Linux

#endif
