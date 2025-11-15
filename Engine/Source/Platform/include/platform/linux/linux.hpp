#ifdef PLATFORM_LINUX

#pragma once

#include "platform/platform.hpp"
#include "platform/window.hpp"

#include "wayland-client-core.h"
#include "wayland-client-protocol.h"
#include "xdh-shell-client-protocol.h"

namespace dirk::Platform::Linux {

DECLARE_LOG_CATEGORY_EXTERN(LogLinux)
DECLARE_LOG_CATEGORY_EXTERN(LogWayland)

struct WaylandState {
    wl_display* display;
    wl_registry* registry;
    wl_compositor* compositor;
    xdg_wm_base* xdgWmBase;
};

class LinuxPlatformImpl : public PlatformImpl {

public:
    LinuxPlatformImpl(const PlatformCreateInfo& createInfo);
    ~LinuxPlatformImpl();
    void pollPlatformEvents() override;

    std::unique_ptr<PlatformWindowImpl> createPlatformWindow(const WindowCreateInfo& createInfo) override;
    vk::SurfaceKHR createTempVulkanSurface(vk::Instance instance) override;

    wl_display* getDisplay() { return state.display; }
    wl_compositor* getCompositor() { return state.compositor; }
    xdg_wm_base* getXdgWmBase() { return state.xdgWmBase; }

private:
    WaylandState state;
};

}; // namespace dirk::Platform::Linux

#endif
