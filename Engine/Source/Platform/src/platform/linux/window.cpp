#ifdef PLATFORM_LINUX

#include "platform/linux/window.hpp"
#include "asserts.hpp"
#include "common.hpp"
#include "logging/logging.hpp"
#include "platform/linux/linux.hpp"

#include "wayland-client-core.h"
#include "wayland-client-protocol.h"

namespace dirk::Platform::Linux {

DEFINE_LOG_CATEGORY(LogLinux)
DEFINE_LOG_CATEGORY(LogWayland)

LinuxWindow::LinuxWindow(const WindowCreateInfo& createInfo, LinuxPlatform& platformImpl)
    : linuxPlatform(platformImpl), size(createInfo.size) {
    wlSurface = wl_compositor_create_surface(linuxPlatform.getWaylandState().compositor);
}

LinuxWindow::~LinuxWindow() {}

vk::SurfaceKHR LinuxWindow::getVulkanSurface(vk::Instance instance) {
    if (vkSurface)
        return vkSurface;

    vk::WaylandSurfaceCreateInfoKHR createInfo;
    createInfo.display = linuxPlatform.getDisplay();
    createInfo.surface = wlSurface;

    vk::detail::DispatchLoaderDynamic dispatcher(instance, vkGetInstanceProcAddr);
    // TODO: use error
    auto err = instance.createWaylandSurfaceKHR(&createInfo, nullptr, &vkSurface, dispatcher);
    check(vkSurface);

    return vkSurface;
}

vk::Extent2D LinuxWindow::getSize() { return size; }
void LinuxWindow::setSize(vk::Extent2D inSize) {}

glm::vec2 LinuxWindow::getPosition() {}
void LinuxWindow::setPosition(const glm::vec2 inPosition) {}

std::string_view LinuxWindow::getTitle() {}
void LinuxWindow::setTitle(std::string_view inTitle) {}

bool LinuxWindow::isFocused() {}
bool LinuxWindow::isMinimized() {}

} // namespace dirk::Platform::Linux

#endif
