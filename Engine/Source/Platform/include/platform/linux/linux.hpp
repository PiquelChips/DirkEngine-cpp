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

class LinuxPlatformImpl : public PlatformImpl {

public:
    LinuxPlatformImpl(const PlatformCreateInfo& createInfo);
    ~LinuxPlatformImpl();
    void pollPlatformEvents() override;

    std::unique_ptr<PlatformWindowImpl> createPlatformWindow(const WindowCreateInfo& createInfo) override;
    vk::SurfaceKHR createTempVulkanSurface(vk::Instance instance) override;

    wl_display* getDisplay() { return display; }
    wl_compositor* getCompositor() { return compositor; }
    xdg_wm_base* getXdgWmBase() { return xdgWmBase; }

private:
    static void globalRegistryHandler(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version);

private:
    wl_display* display;
    wl_registry* registry;
    wl_compositor* compositor;
    xdg_wm_base* xdgWmBase;
};

}; // namespace dirk::Platform::Linux

#endif
