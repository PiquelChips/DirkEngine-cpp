#ifdef PLATFORM_LINUX

#pragma once

#include "platform/platform.hpp"
#include "platform/window.hpp"
#include "wayland-client-core.h"

#include <vector>

namespace dirk::Platform::Linux {

DECLARE_LOG_CATEGORY_EXTERN(LogLinux)
DECLARE_LOG_CATEGORY_EXTERN(LogWayland)

class LinuxPlatform : public PlatformImpl {

public:
    LinuxPlatform(const PlatformCreateInfo& createInfo);
    ~LinuxPlatform();
    virtual void pollPlatformEvents() override;
    virtual std::vector<const char*> getRequiredExtensions() override;
    virtual std::shared_ptr<Window> createWindow(const WindowCreateInfo& createInfo) override;
    virtual void destroyWindow(std::shared_ptr<Window> window) override;
    virtual void focusWindow(std::shared_ptr<Window> window) override;

private:
    wl_display* display;
};

}; // namespace dirk::Platform::Linux

#endif
