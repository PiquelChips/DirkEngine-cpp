#ifdef PLATFORM_LINUX

#include "platform/linux/linux.hpp"
#include "common.hpp"
#include "platform/linux/window.hpp"
#include "platform/platform.hpp"
#include "platform/window.hpp"

#include "vulkan/vulkan_handles.hpp"
#include "wayland-client-core.h"
#include "wayland-client-protocol.h"

namespace dirk::Platform::Linux {

DEFINE_LOG_CATEGORY(LogLinux)
DEFINE_LOG_CATEGORY(LogWayland)

LinuxPlatformImpl::LinuxPlatformImpl(const PlatformCreateInfo& createInfo) {
    display = wl_display_connect(nullptr);
    if (!display) {
        DIRK_LOG(LogWayland, FATAL, "failed to create wayland display")
        return;
    }

    DIRK_LOG(LogWayland, INFO, "connected to display");

    registry = wl_display_get_registry(display);
    if (!registry) {
        DIRK_LOG(LogWayland, FATAL, "failed to get wayland display registry")
    }

    static const struct wl_registry_listener registryListener = {
        .global = globalRegistryHandler,
        .global_remove = [](void* data, struct wl_registry* registry, uint32_t name) {},
    };
    wl_registry_add_listener(registry, &registryListener, this);
    wl_display_roundtrip(display);
}

LinuxPlatformImpl::~LinuxPlatformImpl() {
    if (xdgWmBase) xdg_wm_base_destroy(xdgWmBase);
    if (compositor) wl_compositor_destroy(compositor);
    if (registry) wl_registry_destroy(registry);
    if (display) wl_display_disconnect(display);
}

void LinuxPlatformImpl::pollPlatformEvents() {
    if (wl_display_dispatch(display) == 0) {
        gEngine->exit("platform exit");
    }
}

std::unique_ptr<PlatformWindowImpl> LinuxPlatformImpl::createPlatformWindow(const WindowCreateInfo& createInfo) {
    return std::make_unique<LinuxWindowImpl>(createInfo, *this);
}

vk::SurfaceKHR LinuxPlatformImpl::createTempVulkanSurface(vk::Instance instance) {
    vk::SurfaceKHR vkSurface;
    auto wlSurface = wl_compositor_create_surface(compositor);

    vk::WaylandSurfaceCreateInfoKHR createInfo;
    createInfo.display = display;
    createInfo.surface = wlSurface;

    vk::detail::DispatchLoaderDynamic dispatcher(instance, vkGetInstanceProcAddr);
    auto err = instance.createWaylandSurfaceKHR(&createInfo, nullptr, &vkSurface, dispatcher);
    if (err != vk::Result::eSuccess)
        DIRK_LOG(LogWayland, FATAL, "received error code " << err << " while attempting to create vulkan surface for wayland surface")
    check(vkSurface);

    free(wlSurface);
    wlSurface = nullptr;
    return vkSurface;
}

void LinuxPlatformImpl::globalRegistryHandler(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
    LinuxPlatformImpl* platform = static_cast<LinuxPlatformImpl*>(data);

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        // wl_compositor
        platform->compositor = static_cast<wl_compositor*>(wl_registry_bind(registry, name, &wl_compositor_interface, 6));
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        // xdg_wm_base
        platform->xdgWmBase = static_cast<xdg_wm_base*>(wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
        static const struct xdg_wm_base_listener xdgWmBaseListener = {
            .ping = [](void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial) { xdg_wm_base_pong(xdg_wm_base, serial); },
        };
        xdg_wm_base_add_listener(platform->xdgWmBase, &xdgWmBaseListener, platform);
    }
}

} // namespace dirk::Platform::Linux

#endif
