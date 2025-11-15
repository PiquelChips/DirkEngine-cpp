#ifdef PLATFORM_LINUX

#include "platform/linux/window.hpp"
#include "asserts.hpp"
#include "common.hpp"
#include "logging/logging.hpp"
#include "platform/linux/linux.hpp"

#include "wayland-client-core.h"
#include "wayland-client-protocol.h"

namespace dirk::Platform::Linux {

LinuxWindowImpl::LinuxWindowImpl(const WindowCreateInfo& createInfo, LinuxPlatformImpl& platformImpl)
    : linuxPlatform(platformImpl), size(createInfo.size) {
    wlSurface = wl_compositor_create_surface(linuxPlatform.getWaylandState().compositor);
}

LinuxWindowImpl::~LinuxWindowImpl() {}

vk::SurfaceKHR LinuxWindowImpl::getVulkanSurface(vk::Instance instance) {
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

vk::Extent2D LinuxWindowImpl::getSize() { return size; }
void LinuxWindowImpl::setSize(vk::Extent2D inSize) {}

glm::vec2 LinuxWindowImpl::getPosition() {}
void LinuxWindowImpl::setPosition(const glm::vec2 inPosition) {}

std::string_view LinuxWindowImpl::getTitle() {}
void LinuxWindowImpl::setTitle(std::string_view inTitle) {}

bool LinuxWindowImpl::isFocused() {}
bool LinuxWindowImpl::isMinimized() {}

} // namespace dirk::Platform::Linux

#endif
