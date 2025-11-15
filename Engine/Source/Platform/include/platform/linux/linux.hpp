#ifdef PLATFORM_LINUX

#pragma once

#include "platform/platform.hpp"
#include "platform/window.hpp"

#include "wayland-client-core.h"
#include "wayland-client-protocol.h"
#include "xdh-shell-client-protocol.h"

namespace dirk::Platform::Linux {

DECLARE_LOG_CATEGORY_EXTERN(LogLinux)
DECLARE_LOG_CATEGORY_EXTERN(LogWayland)

class LinuxPlatformImpl : public PlatformImpl {

public:
    LinuxPlatformImpl(const PlatformCreateInfo& createInfo, Platform& platform);
    ~LinuxPlatformImpl();
    void pollPlatformEvents() override;

    std::unique_ptr<PlatformWindowImpl> createPlatformWindow(const WindowCreateInfo& createInfo) override;
    vk::SurfaceKHR createTempVulkanSurface(vk::Instance instance) override;

    wl_display* getDisplay() { return display; }
    wl_compositor* getCompositor() { return compositor; }
    xdg_wm_base* getXdgWmBase() { return xdgWmBase; }

private:
    static void globalRegistryHandler(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version);
    static void seatCapabilities(void* data, wl_seat* seat, uint32_t caps);

    static void pointerEnter(void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface, wl_fixed_t x, wl_fixed_t y);
    static void pointerLeave(void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface);
    static void pointerMotion(void* data, wl_pointer* pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y);
    static void pointerButton(void* data, wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
    static void pointerAxis(void* data, wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value);

    static void keyboardKeymap(void* data, wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size);
    static void keyboardEnter(void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface, wl_array* keys);
    static void keyboardLeave(void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface);
    static void keyboardKey(void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
    static void keyboardModifiers(void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);
    static void keyboardRepeatInfo(void* data, struct wl_keyboard* wl_keyboard, int32_t rate, int32_t delay);

private:
    wl_display* display;
    wl_registry* registry;
    wl_compositor* compositor;
    xdg_wm_base* xdgWmBase;

    // input
    wl_seat* seat;
    wl_pointer* pointer;
    wl_keyboard* keyboard;

    Platform& platform;
};

}; // namespace dirk::Platform::Linux

#endif
