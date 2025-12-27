#ifdef PLATFORM_LINUX

#include "platform/linux/window.hpp"
#include "common.hpp"
#include "logging/logging.hpp"
#include "platform/linux/linux.hpp"
#include "platform/platform.hpp"

#include "imgui.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_wayland.h"
#include "wayland-client-core.h"
#include "wayland-client-protocol.h"
#include "xdg-shell-client-protocol.h"

#include <cstdint>

namespace dirk::Platform::Linux {

#define LOG_WAYLAND_NOT_IMPLEMENTED(feature) ; // DIRK_LOG(LogWayland, WARNING, "{} is not implemented in wayland", feature);

LinuxWindowImpl::LinuxWindowImpl(const WindowCreateInfo& createInfo, LinuxPlatformImpl& platformImpl)
    : linuxPlatform(platformImpl), size(createInfo.size), title(createInfo.title), decorated(createInfo.decorated), pos(createInfo.pos) {
    wlSurface = wl_compositor_create_surface(linuxPlatform.compositor);
    if (!wlSurface)
        DIRK_LOG(LogWayland, FATAL, "failed to create vulkan surface");

    xdgSurface = xdg_wm_base_get_xdg_surface(linuxPlatform.xdgWmBase, wlSurface);
    static const xdg_surface_listener xdgSurfaceListener = {
        .configure = [](void* data, xdg_surface* surface, uint32_t serial) {
            auto* window = static_cast<LinuxWindowImpl*>(data);
            xdg_surface_ack_configure(surface, serial);
            window->configured = true;
        }
    };
    xdg_surface_add_listener(xdgSurface, &xdgSurfaceListener, this);

    xdgToplevel = xdg_surface_get_toplevel(xdgSurface);
    static const xdg_toplevel_listener xdgToplevelListener = {
        .configure = xdg_ToplevelConfigure,
        .close = xdg_ToplevelClose,
        .configure_bounds = [](void*, struct xdg_toplevel*, int32_t, int32_t) {},
        .wm_capabilities = [](void*, struct xdg_toplevel*, struct wl_array*) {},
    };
    xdg_toplevel_add_listener(xdgToplevel, &xdgToplevelListener, this);
    xdg_toplevel_set_title(xdgToplevel, title.data());
    xdg_toplevel_set_app_id(xdgToplevel, "DirkEngine"); // TODO: add the engine application name

    if (createInfo.parent)
        xdg_toplevel_set_parent(xdgToplevel, static_cast<LinuxWindowImpl*>(createInfo.parent)->xdgToplevel);

    // set reasonable constraints to allow resizing
    xdg_toplevel_set_min_size(xdgToplevel, 50, 50); // Reasonable minimum
    xdg_toplevel_set_max_size(xdgToplevel, 0, 0);   // 0 = unlimited

    // TODO: set the decorations

    wl_surface_commit(wlSurface);

    while (!configured)
        wl_display_roundtrip(platformImpl.display);

    wl_display_roundtrip(platformImpl.display); // one last time for input devices

    if (createInfo.focused)
        focus();

    setPosition(createInfo.pos);
}

LinuxWindowImpl::~LinuxWindowImpl() {
    xdg_toplevel_destroy(xdgToplevel);
    xdgToplevel = nullptr;
    xdg_surface_destroy(xdgSurface);
    xdgSurface = nullptr;
    wl_surface_commit(wlSurface);

    wl_surface_destroy(wlSurface);
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
    createInfo.display = linuxPlatform.display;
    createInfo.surface = wlSurface;

    auto err = vkCreateWaylandSurfaceKHR(instance, createInfo, nullptr, surface);
    if (err != VK_SUCCESS)
        DIRK_LOG(LogWayland, FATAL, "received error code {} while attempting to create vulkan surface for wayland surface", (int) err)

    vkSurface = *surface;
    check(vkSurface);
    wl_surface_commit(wlSurface);
}

void LinuxWindowImpl::setPosition(const glm::vec2 inPosition) {
    LOG_WAYLAND_NOT_IMPLEMENTED("setting window position");
    // TODO: start toplevel drag to move into position

}

void LinuxWindowImpl::setTitle(std::string_view inTitle) {
    this->title = inTitle;
    if (!xdgToplevel)
        return;

    xdg_toplevel_set_title(xdgToplevel, title.data());
    wl_surface_commit(wlSurface);
}

bool LinuxWindowImpl::isFocused() {
    // TODO: xdg-activation protocol
    // could also do by tracking keyboard enter & leave events (as we did)
    LOG_WAYLAND_NOT_IMPLEMENTED("focusing windows")
    return false;
}

void LinuxWindowImpl::focus() {
    // TODO: xdg-activation protocol
    LOG_WAYLAND_NOT_IMPLEMENTED("focusing windows")
}

void LinuxWindowImpl::setDecorated(bool inDecorated) {
    this->decorated = inDecorated;
    // TODO: xdg-decoration protocol
    LOG_WAYLAND_NOT_IMPLEMENTED("decorating windows")
}

void LinuxWindowImpl::xdg_ToplevelConfigure(void* data, xdg_toplevel* toplevel, int32_t width, int32_t height, wl_array* states) {
    auto* window = static_cast<LinuxWindowImpl*>(data);

    vk::Extent2D newSize(width, height);
    if (newSize == window->size)
        return;

    window->size = newSize;
    // TODO: as we wait for configuration to finish before returning the constructor,
    // the platform handle is not yet set in the viewport variable. it thus cannot find it
    auto* viewport = ImGui::FindViewportByPlatformHandle(window->wlSurface);
    if (viewport)
        window->linuxPlatform.getPlatform().windowSizeCallback(viewport, window->size);
}

void LinuxWindowImpl::xdg_ToplevelClose(void* data, xdg_toplevel* toplevel) {
    auto* window = static_cast<LinuxWindowImpl*>(data);
    auto* viewport = ImGui::FindViewportByPlatformHandle(window->wlSurface);
    check(viewport);
    window->linuxPlatform.getPlatform().windowCloseCallback(viewport);
}

} // namespace dirk::Platform::Linux

#endif
