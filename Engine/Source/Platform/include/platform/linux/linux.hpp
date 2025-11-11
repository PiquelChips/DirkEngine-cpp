#ifdef PLATFORM_LINUX

#pragma once

#include "wayland-client-core.h"
#include "wayland-client-protocol.h"

#include "platform/platform.hpp"
#include "platform/window.hpp"

#include <vector>

namespace dirk::Platform::Linux {

DECLARE_LOG_CATEGORY_EXTERN(LogLinux)
DECLARE_LOG_CATEGORY_EXTERN(LogWayland)

struct WaylandState {
    wl_compositor* compositor;
};

class LinuxPlatform : public PlatformImpl {

public:
    LinuxPlatform(const PlatformCreateInfo& createInfo);
    ~LinuxPlatform();
    void pollPlatformEvents() override;
    std::unique_ptr<PlatformWindowImpl> createPlatformWindow(const WindowCreateInfo& createInfo) override;
    void destroyWindow(PlatformWindowImpl* window) override;
    void focusWindow(PlatformWindowImpl* window) override;

    WaylandState& getWaylandState() { return state; }
    wl_display* getDisplay() { return display; }

private:
    WaylandState state = { 0 };

    wl_display* display;
    wl_registry* registry;
};

}; // namespace dirk::Platform::Linux

#endif
