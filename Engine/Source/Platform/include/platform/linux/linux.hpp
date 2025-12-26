#ifdef PLATFORM_LINUX

#pragma once

#include "platform/platform.hpp"

#include "vulkan/vulkan_handles.hpp"
#include "wayland-client-core.h"
#include "wayland-client-protocol.h"
#include "xdg-shell-client-protocol.h"
#include "xkbcommon/xkbcommon.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace dirk::Platform::Linux {

DECLARE_LOG_CATEGORY_EXTERN(LogLinux)
DECLARE_LOG_CATEGORY_EXTERN(LogWayland)

class LinuxPlatformImpl : public PlatformImpl {

public:
    LinuxPlatformImpl(const PlatformCreateInfo& createInfo, Platform& platform);
    ~LinuxPlatformImpl();
    void pollPlatformEvents() override;

    std::unique_ptr<PlatformWindowImpl> createPlatformWindow(const WindowCreateInfo& createInfo) override;
    vk::SurfaceKHR createTempSurface(vk::Instance instance) override;

    wl_display* getDisplay() { return display; }
    wl_compositor* getCompositor() { return compositor; }
    xdg_wm_base* getXdgWmBase() { return xdgWmBase; }

    Platform& getPlatform() { return platform; }

    std::string_view getClipboardText() override;
    void setClipboardText(const std::string& text) override;

    static constexpr Input::Key getKeyFromSym(xkb_keysym_t sym);
    static constexpr Input::MouseButton getMouseFromCode(uint32_t code);
    static constexpr Input::KeyState getKeyStateFromCode(uint32_t state);

private:
    static void wl_GlobalRegistryHandler(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version);
    static void wl_SeatCapabilities(void* data, wl_seat* seat, uint32_t caps);

    static void wl_OutputHandleGeometry(void* data, struct wl_output* output, int32_t x, int32_t y, int32_t physicalWidth, int32_t physicalHeight, int32_t subpixel, const char* make, const char* model, int32_t transform);
    static void wl_OutputHandleMode(void* data, struct wl_output* output, uint32_t flags, int32_t width, int32_t height, int32_t refresh);
    static void wl_OutputHandleDone(void* data, struct wl_output* output);
    static void wl_OutputHandleName(void* data, struct wl_output* output, const char* name);
    static void wl_OutputHandleDescription(void* data, struct wl_output* output, const char* description);

    static void wl_PointerEnter(void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface, wl_fixed_t x, wl_fixed_t y);
    static void wl_PointerLeave(void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface);
    static void wl_PointerMotion(void* data, wl_pointer* pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y);
    static void wl_PointerButton(void* data, wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
    static void wl_PointerAxis(void* data, wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value);

    static void wl_KeyboardKeymap(void* data, wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size);
    static void wl_KeyboardEnter(void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface, wl_array* keys);
    static void wl_KeyboardLeave(void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface);
    static void wl_KeyboardKey(void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
    static void wl_KeyboardModifiers(void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);

private:
    wl_display* display;
    wl_registry* registry;
    wl_compositor* compositor;
    xdg_wm_base* xdgWmBase;

    // input
    wl_seat* seat;
    wl_pointer* pointer;
    wl_keyboard* keyboard;

    xkb_context* xkbContext;
    xkb_keymap* xkbKeymap;
    xkb_state* xkbState;

    Platform& platform;
};

}; // namespace dirk::Platform::Linux

#endif
