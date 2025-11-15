#ifdef PLATFORM_LINUX

#include "platform/linux/window.hpp"
#include "common.hpp"
#include "logging/logging.hpp"
#include "platform/linux/linux.hpp"

#include "wayland-client-core.h"
#include "wayland-client-protocol.h"
#include "xdh-shell-client-protocol.h"

namespace dirk::Platform::Linux {

LinuxWindowImpl::LinuxWindowImpl(const WindowCreateInfo& createInfo, LinuxPlatformImpl& platformImpl)
    : linuxPlatform(platformImpl), size(createInfo.size) {
    wlSurface = wl_compositor_create_surface(linuxPlatform.getCompositor());
    if (!wlSurface)
        DIRK_LOG(LogWayland, FATAL, "failed to create vulkan surface");

    xdgSurface = xdg_wm_base_get_xdg_surface(linuxPlatform.getXdgWmBase(), wlSurface);
    // xdg_surface_add_listener(state.xdg_surface, &xdg_surface_listener, &state);
    xdgToplevel = xdg_surface_get_toplevel(xdgSurface);

    setSize(createInfo.size);
    setTitle(createInfo.title);
    focus(createInfo.focused);
    // TODO: visible
    // TODO: decorated
    // TODO: floating
}

LinuxWindowImpl::~LinuxWindowImpl() {}

vk::SurfaceKHR LinuxWindowImpl::getVulkanSurface(vk::Instance instance) {
    if (vkSurface)
        return vkSurface;

    vk::WaylandSurfaceCreateInfoKHR createInfo;
    createInfo.display = linuxPlatform.getDisplay();
    createInfo.surface = wlSurface;

    vk::detail::DispatchLoaderDynamic dispatcher(instance, vkGetInstanceProcAddr);
    auto err = instance.createWaylandSurfaceKHR(&createInfo, nullptr, &vkSurface, dispatcher);
    if (err != vk::Result::eSuccess)
        DIRK_LOG(LogWayland, FATAL, "received error code " << err << " while attempting to create vulkan surface for wayland surface")
    check(vkSurface);

    return vkSurface;
}

vk::Extent2D LinuxWindowImpl::getSize() { return size; }

void LinuxWindowImpl::setSize(vk::Extent2D inSize) {
    this->size = inSize;
    // TODO: resize window
}

glm::vec2 LinuxWindowImpl::getPosition() {}
void LinuxWindowImpl::setPosition(const glm::vec2 inPosition) {}

std::string_view LinuxWindowImpl::getTitle() { return title; }

void LinuxWindowImpl::setTitle(std::string_view inTitle) {
    this->title = inTitle;
    xdg_toplevel_set_title(xdgToplevel, title.data());
}

bool LinuxWindowImpl::isFocused() { return focused; }

void LinuxWindowImpl::focus(bool inFocused) {
    this->focused = inFocused;
    // TODO: focus the window
}

bool LinuxWindowImpl::isMinimized() { return minimized; }

void LinuxWindowImpl::minimize(bool isMinimized) {
    this->minimized = minimized;
    // TODO: minimize the window
}

} // namespace dirk::Platform::Linux

#endif
