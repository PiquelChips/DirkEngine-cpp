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
        state->compositor = static_cast<wl_compositor*>(
            wl_registry_bind(registry, name, &wl_compositor_interface, 6));
    }
}

static void registry_handle_global_remove(void* data, struct wl_registry* registry, uint32_t name) {}

static const struct wl_registry_listener
    registryListener = {
        .global = registry_handle_global,
        .global_remove = registry_handle_global_remove,
    };

LinuxPlatform::LinuxPlatform(const PlatformCreateInfo& createInfo) {
    display = wl_display_connect(nullptr);
    check(display);
    DIRK_LOG(LogWayland, INFO, "connected to display");
    registry = wl_display_get_registry(display);
    check(registry);
    wl_registry_add_listener(registry, &registryListener, &state);
    wl_display_roundtrip(display);
}

LinuxPlatform::~LinuxPlatform() {
    wl_display_disconnect(display);
}

void LinuxPlatform::pollPlatformEvents() {
    // TODO: handle return value
    wl_display_dispatch(display);
}

std::unique_ptr<PlatformWindowImpl> LinuxPlatform::createPlatformWindow(const WindowCreateInfo& createInfo) {
    return std::make_unique<LinuxWindow>(LinuxWindowCreateInfo{
        .platformImpl = this,
        .createInfo = createInfo,
    });
}

vk::SurfaceKHR LinuxPlatform::createTempVulkanSurface(vk::Instance instance) {
    vk::SurfaceKHR vkSurface;
    auto wlSurface = wl_compositor_create_surface(state.compositor);

    vk::WaylandSurfaceCreateInfoKHR createInfo;
    createInfo.display = display;
    createInfo.surface = wlSurface;

    vk::detail::DispatchLoaderDynamic dispatcher(instance, vkGetInstanceProcAddr);
    // TODO: use error
    auto err = instance.createWaylandSurfaceKHR(&createInfo, nullptr, &vkSurface, dispatcher);
    check(vkSurface);

    free(wlSurface);
    wlSurface = nullptr;
    return vkSurface;
}

// TODO: destroy window
void LinuxPlatform::destroyWindow(PlatformWindowImpl* window) {}

void LinuxPlatform::focusWindow(PlatformWindowImpl* window) {
    auto linuxWindow = static_cast<LinuxWindow*>(window);
}

} // namespace dirk::Platform::Linux

#endif
