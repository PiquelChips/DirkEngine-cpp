#include "platform/platform.hpp"
#ifdef PLATFORM_LINUX

#include "platform/linux/linux.hpp"

namespace dirk::Platform::Linux {

LinuxPlatform::LinuxPlatform(const PlatformCreateInfo& createInfo) {
    display = wl_display_connect(nullptr);
    DIRK_LOG(LogWayland, INFO, "connected to display")
}

LinuxPlatform::~LinuxPlatform() {
    wl_display_disconnect(display);
}

void LinuxPlatform::pollPlatformEvents() {
    // TODO: handle return value
    wl_display_dispatch(display);
}

std::vector<const char*> LinuxPlatform::getRequiredExtensions() {}

} // namespace dirk::Platform::Linux

#endif
