#include <cstring>
#ifdef PLATFORM_LINUX

#include "common.hpp"
#include "platform/linux/linux.hpp"
#include "platform/linux/window.hpp"
#include "platform/platform.hpp"
#include "platform/window.hpp"

#include "vulkan/vulkan_handles.hpp"
#include "wayland-client-core.h"
#include "wayland-client-protocol.h"

namespace dirk::Platform::Linux {

DEFINE_LOG_CATEGORY(LogLinux)
DEFINE_LOG_CATEGORY(LogWayland)

LinuxPlatformImpl::LinuxPlatformImpl(const PlatformCreateInfo& createInfo, Platform& platform)
    : platform(platform) {
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
        .global = wl_GlobalRegistryHandler,
        .global_remove = [](void* data, struct wl_registry* registry, uint32_t name) {},
    };
    wl_registry_add_listener(registry, &registryListener, this);
    wl_display_roundtrip(display);
}

LinuxPlatformImpl::~LinuxPlatformImpl() {
    if (keyboard) wl_keyboard_destroy(keyboard);
    if (pointer) wl_pointer_destroy(pointer);
    if (seat) wl_seat_destroy(seat);

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

void LinuxPlatformImpl::wl_GlobalRegistryHandler(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
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
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        platform->seat = static_cast<wl_seat*>(wl_registry_bind(registry, name, &wl_seat_interface, 7));
        static const wl_seat_listener seatListener = {
            wl_SeatCapabilities,
        };
        wl_seat_add_listener(platform->seat, &seatListener, platform);
    }
}

void LinuxPlatformImpl::wl_SeatCapabilities(void* data, wl_seat* seat, uint32_t caps) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);

    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !platform->pointer) {
        platform->pointer = wl_seat_get_pointer(seat);

        static const wl_pointer_listener pointerListener = {
            wl_PointerEnter,
            wl_PointerLeave,
            wl_PointerMotion,
            wl_PointerButton,
            wl_PointerAxis,
        };
        wl_pointer_add_listener(platform->pointer, &pointerListener, platform);
    }

    if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !platform->keyboard) {
        platform->keyboard = wl_seat_get_keyboard(seat);

        static const wl_keyboard_listener keyboardListener = {
            wl_KeyboardKeymap,
            wl_KeyboardEnter,
            wl_KeyboardLeave,
            wl_KeyboardKey,
            wl_KeyboardModifiers,
            wl_KeyboardRepeatInfo,
        };
        wl_keyboard_add_listener(platform->keyboard, &keyboardListener, platform);
    }
}

void LinuxPlatformImpl::wl_PointerEnter(void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface, wl_fixed_t x, wl_fixed_t y) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
}

void LinuxPlatformImpl::wl_PointerLeave(void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
}

void LinuxPlatformImpl::wl_PointerMotion(void* data, wl_pointer* pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
}

void LinuxPlatformImpl::wl_PointerButton(void* data, wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
}

void LinuxPlatformImpl::wl_PointerAxis(void* data, wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
}

void LinuxPlatformImpl::wl_KeyboardKeymap(void* data, wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
}

void LinuxPlatformImpl::wl_KeyboardEnter(void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface, wl_array* keys) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
}

void LinuxPlatformImpl::wl_KeyboardLeave(void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
}

void LinuxPlatformImpl::wl_KeyboardKey(void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
}

void LinuxPlatformImpl::wl_KeyboardModifiers(void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
}

void LinuxPlatformImpl::wl_KeyboardRepeatInfo(void* data, wl_keyboard* wl_keyboard, int32_t rate, int32_t delay) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
}

} // namespace dirk::Platform::Linux

#endif
