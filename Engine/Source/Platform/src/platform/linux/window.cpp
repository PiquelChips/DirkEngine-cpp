#ifdef PLATFORM_LINUX

#include "common.hpp"
#include "logging/logging.hpp"
#include "platform/linux/linux.hpp"
#include "platform/linux/window.hpp"

#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_wayland.h"
#include "wayland-client-core.h"
#include "wayland-client-protocol.h"
#include "xdg-shell-client-protocol.h"

namespace dirk::Platform::Linux {

#define LOG_WAYLAND_NOT_IMPLEMENTED(feature) DIRK_LOG(LogWayland, WARNING, feature << " is not implemented in wayland");

LinuxWindowImpl::LinuxWindowImpl(const WindowCreateInfo& createInfo, LinuxPlatformImpl& platformImpl)
    : linuxPlatform(platformImpl), size(createInfo.size) {
    wlSurface = wl_compositor_create_surface(linuxPlatform.getCompositor());
    if (!wlSurface)
        DIRK_LOG(LogWayland, FATAL, "failed to create vulkan surface");

    setSize(createInfo.size);
    setTitle(createInfo.title);
    if (createInfo.focused)
        focus();
    setDecorated(createInfo.decorated);
}

LinuxWindowImpl::~LinuxWindowImpl() {
    hide();
    if (wlSurface) wl_surface_destroy(wlSurface);
}

void LinuxWindowImpl::show() {
    xdgSurface = xdg_wm_base_get_xdg_surface(linuxPlatform.getXdgWmBase(), wlSurface);
    static const xdg_surface_listener xdgSurfaceListener = {
        .configure = [](void* data, xdg_surface* surface, uint32_t serial) {
            auto* window = static_cast<LinuxWindowImpl*>(data);
            xdg_surface_ack_configure(surface, serial);
        }
    };
    xdg_surface_add_listener(xdgSurface, &xdgSurfaceListener, this);

    xdgToplevel = xdg_surface_get_toplevel(xdgSurface);

    static const xdg_toplevel_listener xdgToplevelListener = {
        .configure = [](void* data, xdg_toplevel* toplevel, int32_t width, int32_t height, wl_array* states) {
            auto* window = static_cast<LinuxWindowImpl*>(data);

            if (width < 0 && height < 0) {
                window->minimized = true;
            }

            window->setSize(vk::Extent2D(width, height));
            // TODO: use platform window size callback
        },
        .close = [](void* data, xdg_toplevel* toplevel) {
            auto* window = static_cast<LinuxWindowImpl*>(data);
            // TODO: use platform window close callback
        },
        .configure_bounds = [](void*, struct xdg_toplevel*, int32_t, int32_t) {},
        .wm_capabilities = [](void*, struct xdg_toplevel*, struct wl_array*) {},
    };
    xdg_toplevel_add_listener(xdgToplevel, &xdgToplevelListener, this);

    wl_surface_commit(wlSurface);

    setSize(this->size);
    setTitle(this->title);
    setDecorated(this->decorated);
}

void LinuxWindowImpl::hide() {
    xdg_toplevel_destroy(xdgToplevel);
    xdgToplevel = nullptr;
    xdg_surface_destroy(xdgSurface);
    xdgSurface = nullptr;
}

vk::SurfaceKHR LinuxWindowImpl::getVulkanSurface(vk::Instance instance) {
    if (vkSurface)
        return vkSurface;

    VkSurfaceKHR surf;
    createVulkanSurface(instance, &surf);
    return vkSurface;
}

void LinuxWindowImpl::createVulkanSurface(VkInstance instance, VkSurfaceKHR* surface) {
    vk::WaylandSurfaceCreateInfoKHR createInfo;
    createInfo.display = linuxPlatform.getDisplay();
    createInfo.surface = wlSurface;

    auto err = vkCreateWaylandSurfaceKHR(instance, createInfo, nullptr, surface);
    if (err != VK_SUCCESS)
        DIRK_LOG(LogWayland, FATAL, "received error code " << err << " while attempting to create vulkan surface for wayland surface")

    vkSurface = *surface;
    check(vkSurface);
}

void LinuxWindowImpl::setSize(vk::Extent2D inSize) {
    this->size = inSize;
    // TODO: resize window if window obj valid
}

// clang-format off
glm::vec2 LinuxWindowImpl::getPosition() { LOG_WAYLAND_NOT_IMPLEMENTED("getting window position"); return {0, 0}; }
void LinuxWindowImpl::setPosition(const glm::vec2 inPosition) { LOG_WAYLAND_NOT_IMPLEMENTED("setting window position"); }
// clang-format on

void LinuxWindowImpl::setTitle(std::string_view inTitle) {
    this->title = inTitle;
    if (!xdgToplevel)
        return;

    xdg_toplevel_set_title(xdgToplevel, title.data());
    wl_surface_commit(wlSurface);
}

bool LinuxWindowImpl::isFocused() {
    // TODO: xdg-activation protocol
    return false;
}

void LinuxWindowImpl::focus() {
    // TODO: xdg-activation protocol
}

void LinuxWindowImpl::setDecorated(bool inDecorated) {
    this->decorated = inDecorated;
    // TODO: xdg-decoration protocol
}

} // namespace dirk::Platform::Linux

#endif
