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

static void registry_handle_global(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
    WaylandState* state = static_cast<WaylandState*>(data);

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        // wl_compositor
        state->compositor = static_cast<wl_compositor*>(wl_registry_bind(registry, name, &wl_compositor_interface, 6));
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        // xdg_wm_base
        state->xdgWmBase = static_cast<xdg_wm_base*>(wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
        static const struct xdg_wm_base_listener xdg_wm_base_listener = {
            .ping = [](void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial) { xdg_wm_base_pong(xdg_wm_base, serial); },
        };
        xdg_wm_base_add_listener(state->xdgWmBase, &xdg_wm_base_listener, state);
    }
}

static const struct wl_registry_listener
    registryListener = {
        .global = registry_handle_global,
        .global_remove = [](void* data, struct wl_registry* registry, uint32_t name) {},
    };

LinuxPlatformImpl::LinuxPlatformImpl(const PlatformCreateInfo& createInfo) {
    state.display = wl_display_connect(nullptr);
    if (!state.display) {
        DIRK_LOG(LogWayland, FATAL, "failed to create wayland display")
        return;
    }

    DIRK_LOG(LogWayland, INFO, "connected to display");

    state.registry = wl_display_get_registry(state.display);
    if (!state.registry) {
        DIRK_LOG(LogWayland, FATAL, "failed to get wayland display registry")
    }

    wl_registry_add_listener(state.registry, &registryListener, &state);
    wl_display_roundtrip(state.display);
}

LinuxPlatformImpl::~LinuxPlatformImpl() {
    if (state.xdgWmBase) xdg_wm_base_destroy(state.xdgWmBase);
    if (state.compositor) wl_compositor_destroy(state.compositor);
    if (state.registry) wl_registry_destroy(state.registry);
    if (state.display) wl_display_disconnect(state.display);
}

void LinuxPlatformImpl::pollPlatformEvents() {
    if (wl_display_dispatch(state.display) == 0) {
        gEngine->exit("platform exit");
    }
}

std::unique_ptr<PlatformWindowImpl> LinuxPlatformImpl::createPlatformWindow(const WindowCreateInfo& createInfo) {
    return std::make_unique<LinuxWindowImpl>(createInfo, *this);
}

vk::SurfaceKHR LinuxPlatformImpl::createTempVulkanSurface(vk::Instance instance) {
    vk::SurfaceKHR vkSurface;
    auto wlSurface = wl_compositor_create_surface(state.compositor);

    vk::WaylandSurfaceCreateInfoKHR createInfo;
    createInfo.display = state.display;
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

} // namespace dirk::Platform::Linux

#endif
