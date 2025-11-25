#ifdef PLATFORM_LINUX

#include "platform/linux/linux.hpp"
#include "common.hpp"
#include "input/keys.hpp"
#include "platform/linux/window.hpp"
#include "platform/monitor.hpp"
#include "platform/platform.hpp"
#include "platform/window.hpp"

#include "linux/input-event-codes.h"
#include "vulkan/vulkan_handles.hpp"
#include "wayland-client-core.h"
#include "wayland-client-protocol.h"
#include "wayland-util.h"
#include "xkbcommon/xkbcommon-keysyms.h"
#include "xkbcommon/xkbcommon.h"

#include <cstdint>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>

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
        return;
    }

    static const struct wl_registry_listener registryListener = {
        .global = wl_GlobalRegistryHandler,
        .global_remove = [](void* data, struct wl_registry* registry, uint32_t name) {},
    };
    wl_registry_add_listener(registry, &registryListener, this);
    wl_display_roundtrip(display);

    xkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
}

LinuxPlatformImpl::~LinuxPlatformImpl() {
    if (xkbState) xkb_state_unref(xkbState);
    if (xkbKeymap) xkb_keymap_unref(xkbKeymap);
    if (xkbContext) xkb_context_unref(xkbContext);

    if (keyboard) wl_keyboard_destroy(keyboard);
    if (pointer) wl_pointer_destroy(pointer);
    if (seat) wl_seat_destroy(seat);

    if (xdgWmBase) xdg_wm_base_destroy(xdgWmBase);
    if (compositor) wl_compositor_destroy(compositor);
    if (registry) wl_registry_destroy(registry);
    if (display) wl_display_disconnect(display);
}

void LinuxPlatformImpl::pollPlatformEvents() {
    if (wl_display_dispatch_pending(display) == -1) {
        gEngine->exit("unable to poll platform events. wayland probably disconnected");
        return;
    }

    if (wl_display_flush(display) == -1) {
        gEngine->exit("unable to flush wayland display");
        return;
    }
}

std::unique_ptr<PlatformWindowImpl> LinuxPlatformImpl::createPlatformWindow(const WindowCreateInfo& createInfo) {
    return std::make_unique<LinuxWindowImpl>(createInfo, *this);
}

Window& LinuxPlatformImpl::getWindowWithSurface(wl_surface* surface) {
    for (auto& window : platform.getWindows()) {
        if (static_cast<wl_surface*>(window->getPlatformHandle()) == surface)
            return *window;
    }
    DIRK_LOG(LogWayland, FATAL, "unable to find window with surface");
    return platform.getMainWindow();
}

vk::SurfaceKHR LinuxPlatformImpl::createTempSurface(vk::Instance instance) {
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
        platform->compositor = static_cast<wl_compositor*>(wl_registry_bind(registry, name, &wl_compositor_interface, version));
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        // xdg_wm_base
        platform->xdgWmBase = static_cast<xdg_wm_base*>(wl_registry_bind(registry, name, &xdg_wm_base_interface, version));
        static const struct xdg_wm_base_listener xdgWmBaseListener = {
            .ping = [](void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial) { xdg_wm_base_pong(xdg_wm_base, serial); },
        };
        xdg_wm_base_add_listener(platform->xdgWmBase, &xdgWmBaseListener, platform);
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        // wl_seat
        platform->seat = static_cast<wl_seat*>(wl_registry_bind(registry, name, &wl_seat_interface, version));
        static const wl_seat_listener seatListener = {
            .capabilities = wl_SeatCapabilities,
            .name = [](void*, struct wl_seat*, const char* name) { DIRK_LOG(LogWayland, INFO, "new seat " << name); },
        };
        wl_seat_add_listener(platform->seat, &seatListener, platform);
    } else if (strcmp(interface, wl_output_interface.name) == 0) {
        // wl_output
        wl_output* output = static_cast<wl_output*>(wl_registry_bind(platform->registry, name, &wl_output_interface, version));
        static const struct wl_output_listener outputListener = {
            .geometry = wl_OutputHandleGeometry,
            .mode = wl_OutputHandleMode,
            .done = wl_OutputHandleDone,
            .scale = [](void*, struct wl_output*, int32_t) {},
            .name = wl_OutputHandleName,
            .description = [](void*, struct wl_output*, const char*) {},

        };
        wl_output_add_listener(output, &outputListener, &platform->platform.createMonitor(output));
    }
}

void LinuxPlatformImpl::wl_SeatCapabilities(void* data, wl_seat* seat, uint32_t caps) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);

    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !platform->pointer) {
        platform->pointer = wl_seat_get_pointer(seat);

        static const wl_pointer_listener pointerListener = {
            .enter = wl_PointerEnter,
            .leave = [](void*, struct wl_pointer*, uint32_t, struct wl_surface*) {},
            .motion = wl_PointerMotion,
            .button = wl_PointerButton,
            .axis = wl_PointerAxis,
            .frame = [](void*, struct wl_pointer*) {},
        };
        wl_pointer_add_listener(platform->pointer, &pointerListener, platform);
    }

    if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !platform->keyboard) {
        platform->keyboard = wl_seat_get_keyboard(seat);

        static const wl_keyboard_listener keyboardListener = {
            .keymap = wl_KeyboardKeymap,
            .enter = wl_KeyboardEnter,
            .leave = [](void*, struct wl_keyboard*, uint32_t, struct wl_surface*) {},
            .key = wl_KeyboardKey,
            .modifiers = wl_KeyboardModifiers,
            .repeat_info = [](void* data, struct wl_keyboard* keyboard, int32_t rate, int32_t delay) {},
        };
        wl_keyboard_add_listener(platform->keyboard, &keyboardListener, platform);
    }
}

void LinuxPlatformImpl::wl_OutputHandleGeometry(void* data, struct wl_output* output, int32_t x, int32_t y, int32_t physicalWidth, int32_t physicalHeight, int32_t subpixel, const char* make, const char* model, int32_t transform) {
    auto* monitor = static_cast<Monitor*>(data);
    check(monitor);
    check(monitor->getPlatformHandle() == output);
    monitor->setPosition({ x, y });
}

void LinuxPlatformImpl::wl_OutputHandleMode(void* data, struct wl_output* output, uint32_t flags, int32_t width, int32_t height, int32_t refresh) {
    auto* monitor = static_cast<Monitor*>(data);
    check(monitor);
    check(monitor->getPlatformHandle() == output);

    VideoMode mode{
        .size = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) },
        .refreshRate = static_cast<uint32_t>(refresh / 1000.f),
    };
    monitor->setVideoMode(mode);
}

void LinuxPlatformImpl::wl_OutputHandleDone(void* data, struct wl_output* output) {
    auto* monitor = static_cast<Monitor*>(data);
    check(monitor);
    check(monitor->getPlatformHandle() == output);

    monitor->getPlatform().updateMonitors();
}

void LinuxPlatformImpl::wl_OutputHandleName(void* data, struct wl_output* output, const char* name) {
    auto* monitor = static_cast<Monitor*>(data);
    check(monitor);
    check(monitor->getPlatformHandle() == output);
    monitor->setName(name);
    DIRK_LOG(LogWayland, INFO, "new monitor " << name)
}

void LinuxPlatformImpl::wl_KeyboardKeymap(void* data, wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);

    check(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1);
    char* keymapStr = static_cast<char*>(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));
    check(keymapStr != MAP_FAILED);

    if (platform->xkbKeymap) {
        xkb_keymap_unref(platform->xkbKeymap);
    }
    platform->xkbKeymap = xkb_keymap_new_from_string(platform->xkbContext, keymapStr, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(static_cast<void*>(keymapStr), size);
    close(fd);

    if (platform->xkbState) {
        xkb_state_unref(platform->xkbState);
    }
    platform->xkbState = xkb_state_new(platform->xkbKeymap);
}

void LinuxPlatformImpl::wl_PointerEnter(void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface, wl_fixed_t x, wl_fixed_t y) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
    check(platform->pointer == pointer);
    auto& window = platform->getWindowWithSurface(surface);

    platform->platform.focusWindowCallback(window);
}

void LinuxPlatformImpl::wl_PointerMotion(void* data, wl_pointer* pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
    check(platform->pointer == pointer);
    auto& window = platform->platform.getFocusedWindow();

    auto posX = static_cast<float>(wl_fixed_to_double(x));
    auto posY = static_cast<float>(wl_fixed_to_double(y));
    platform->platform.cursorPosCallback(window, { posX, posY });
}

void LinuxPlatformImpl::wl_PointerButton(void* data, wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t inButton, uint32_t inState) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
    check(platform->pointer == pointer);
    auto& window = platform->platform.getFocusedWindow();

    Input::KeyState state = getKeyStateFromCode(inState);
    Input::MouseButton button = getMouseFromCode(inButton);

    platform->platform.mouseButtonCallback(window, button, state);
}

void LinuxPlatformImpl::wl_PointerAxis(void* data, wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
    check(platform->pointer == pointer);
    auto& window = platform->platform.getFocusedWindow();

    double delta = wl_fixed_to_double(value);
    if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
        platform->platform.mouseScrollCallback(window, { 0.f, -delta / 10.f });
    } else {
        platform->platform.mouseScrollCallback(window, { -delta / 10.f, 0.f });
    }
}

void LinuxPlatformImpl::wl_KeyboardEnter(void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface, wl_array* keys) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
    check(platform->keyboard == keyboard);
    auto& window = platform->getWindowWithSurface(surface);

    platform->platform.focusWindowCallback(window);
}

void LinuxPlatformImpl::wl_KeyboardKey(void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t inKey, uint32_t inState) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
    check(platform->keyboard == keyboard);
    check(platform->xkbState);
    auto& window = platform->platform.getFocusedWindow();

    Input::KeyState state = getKeyStateFromCode(inState);

    xkb_keysym_t sym = xkb_state_key_get_one_sym(platform->xkbState, inKey);
    Input::Key key = getKeyFromSym(sym);

    platform->platform.keyCallback(window, key, state);

    if (state == Input::KeyState::Pressed) {
        uint32_t c = xkb_state_key_get_utf32(platform->xkbState, sym);
        platform->platform.charCallback(window, c);
    }
}

void LinuxPlatformImpl::wl_KeyboardModifiers(void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t depressed, uint32_t latched, uint32_t locked, uint32_t group) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
    check(platform->keyboard == keyboard);
    auto& window = platform->platform.getFocusedWindow();

    xkb_state_update_mask(platform->xkbState, depressed, latched, locked, 0, 0, group);
}

constexpr Input::Key LinuxPlatformImpl::getKeyFromSym(xkb_keysym_t sym) {
    // clang-format off
    switch (sym)
    {
    case XKB_KEY_Tab: return Input::Key::Tab;
    case XKB_KEY_Left: return Input::Key::Left;
    case XKB_KEY_Right: return Input::Key::Right;
    case XKB_KEY_Up: return Input::Key::Up;
    case XKB_KEY_Down: return Input::Key::Down;
    case XKB_KEY_Page_Up: return Input::Key::PageUp;
    case XKB_KEY_Page_Down: return Input::Key::PageDown;
    case XKB_KEY_Home: return Input::Key::Home;
    case XKB_KEY_End: return Input::Key::End;
    case XKB_KEY_Insert: return Input::Key::Insert;
    case XKB_KEY_Delete: return Input::Key::Delete;
    case XKB_KEY_BackSpace: return Input::Key::Backspace;
    case XKB_KEY_space: return Input::Key::Space;
    case XKB_KEY_Return: return Input::Key::Enter;
    case XKB_KEY_Escape: return Input::Key::Escape;
    case XKB_KEY_apostrophe: return Input::Key::Apostrophe;
    case XKB_KEY_comma: return Input::Key::Comma;
    case XKB_KEY_minus: return Input::Key::Minus;
    case XKB_KEY_period: return Input::Key::Period;
    case XKB_KEY_slash: return Input::Key::Slash;
    case XKB_KEY_semicolon: return Input::Key::Semicolon;
    case XKB_KEY_equal: return Input::Key::Equal;
    case XKB_KEY_leftanglebracket: return Input::Key::LeftBracket;
    case XKB_KEY_backslash: return Input::Key::Backslash;
    case XKB_KEY_rightanglebracket: return Input::Key::RightBracket;
    case XKB_KEY_grave: return Input::Key::GraveAccent;
    case XKB_KEY_Caps_Lock: return Input::Key::CapsLock;
    case XKB_KEY_Scroll_Lock: return Input::Key::ScrollLock;
    case XKB_KEY_Num_Lock: return Input::Key::NumLock;
    case XKB_KEY_Print: return Input::Key::PrintScreen;
    case XKB_KEY_Pause: return Input::Key::Pause;
    case XKB_KEY_KP_0: return Input::Key::KP0;
    case XKB_KEY_KP_1: return Input::Key::KP1;
    case XKB_KEY_KP_2: return Input::Key::KP2;
    case XKB_KEY_KP_3: return Input::Key::KP3;
    case XKB_KEY_KP_4: return Input::Key::KP4;
    case XKB_KEY_KP_5: return Input::Key::KP5;
    case XKB_KEY_KP_6: return Input::Key::KP6;
    case XKB_KEY_KP_7: return Input::Key::KP7;
    case XKB_KEY_KP_8: return Input::Key::KP8;
    case XKB_KEY_KP_9: return Input::Key::KP9;
    case XKB_KEY_KP_Decimal: return Input::Key::KPDecimal;
    case XKB_KEY_KP_Divide: return Input::Key::KPDivide;
    case XKB_KEY_KP_Multiply: return Input::Key::KPMultiply;
    case XKB_KEY_KP_Subtract: return Input::Key::KPSubtract;
    case XKB_KEY_KP_Add: return Input::Key::KPAdd;
    case XKB_KEY_KP_Enter: return Input::Key::KPEnter;
    case XKB_KEY_KP_Equal: return Input::Key::KPEqual;
    case XKB_KEY_Shift_L: return Input::Key::LeftShift;
    case XKB_KEY_Control_L: return Input::Key::LeftControl;
    case XKB_KEY_Alt_L: return Input::Key::LeftAlt;
    case XKB_KEY_Super_L: return Input::Key::LeftSuper;
    case XKB_KEY_Shift_R: return Input::Key::RightShift;
    case XKB_KEY_Control_R: return Input::Key::RightControl;
    case XKB_KEY_Alt_R: return Input::Key::RightAlt;
    case XKB_KEY_Super_R: return Input::Key::RightSuper;
    case XKB_KEY_Menu: return Input::Key::Menu;
    case XKB_KEY_0: return Input::Key::D0;
    case XKB_KEY_1: return Input::Key::D1;
    case XKB_KEY_2: return Input::Key::D2;
    case XKB_KEY_3: return Input::Key::D3;
    case XKB_KEY_4: return Input::Key::D4;
    case XKB_KEY_5: return Input::Key::D5;
    case XKB_KEY_6: return Input::Key::D6;
    case XKB_KEY_7: return Input::Key::D7;
    case XKB_KEY_8: return Input::Key::D8;
    case XKB_KEY_9: return Input::Key::D9;
    case XKB_KEY_A: return Input::Key::A;
    case XKB_KEY_B: return Input::Key::B;
    case XKB_KEY_C: return Input::Key::C;
    case XKB_KEY_D: return Input::Key::D;
    case XKB_KEY_E: return Input::Key::E;
    case XKB_KEY_F: return Input::Key::F;
    case XKB_KEY_G: return Input::Key::G;
    case XKB_KEY_H: return Input::Key::H;
    case XKB_KEY_I: return Input::Key::I;
    case XKB_KEY_J: return Input::Key::J;
    case XKB_KEY_K: return Input::Key::K;
    case XKB_KEY_L: return Input::Key::L;
    case XKB_KEY_M: return Input::Key::M;
    case XKB_KEY_N: return Input::Key::N;
    case XKB_KEY_O: return Input::Key::O;
    case XKB_KEY_P: return Input::Key::P;
    case XKB_KEY_Q: return Input::Key::Q;
    case XKB_KEY_R: return Input::Key::R;
    case XKB_KEY_S: return Input::Key::S;
    case XKB_KEY_T: return Input::Key::T;
    case XKB_KEY_U: return Input::Key::U;
    case XKB_KEY_V: return Input::Key::V;
    case XKB_KEY_W: return Input::Key::W;
    case XKB_KEY_X: return Input::Key::X;
    case XKB_KEY_Y: return Input::Key::Y;
    case XKB_KEY_Z: return Input::Key::Z;
    case XKB_KEY_F1: return Input::Key::F1;
    case XKB_KEY_F2: return Input::Key::F2;
    case XKB_KEY_F3: return Input::Key::F3;
    case XKB_KEY_F4: return Input::Key::F4;
    case XKB_KEY_F5: return Input::Key::F5;
    case XKB_KEY_F6: return Input::Key::F6;
    case XKB_KEY_F7: return Input::Key::F7;
    case XKB_KEY_F8: return Input::Key::F8;
    case XKB_KEY_F9: return Input::Key::F9;
    case XKB_KEY_F10: return Input::Key::F10;
    case XKB_KEY_F11: return Input::Key::F11;
    case XKB_KEY_F12: return Input::Key::F12;
    case XKB_KEY_F13: return Input::Key::F13;
    case XKB_KEY_F14: return Input::Key::F14;
    case XKB_KEY_F15: return Input::Key::F15;
    case XKB_KEY_F16: return Input::Key::F16;
    case XKB_KEY_F17: return Input::Key::F17;
    case XKB_KEY_F18: return Input::Key::F18;
    case XKB_KEY_F19: return Input::Key::F19;
    case XKB_KEY_F20: return Input::Key::F20;
    case XKB_KEY_F21: return Input::Key::F21;
    case XKB_KEY_F22: return Input::Key::F22;
    case XKB_KEY_F23: return Input::Key::F23;
    case XKB_KEY_F24: return Input::Key::F24;
    default: return Input::Key::Unknown;
    }
    // clang-format on
}

constexpr Input::MouseButton LinuxPlatformImpl::getMouseFromCode(uint32_t button) {
    // clang-format off
    switch (button) {
    case BTN_LEFT: return Input::MouseButton::Left;
    case BTN_RIGHT: return Input::MouseButton::Right;
    case BTN_MIDDLE: return Input::MouseButton::Middle;
    case BTN_0: return Input::MouseButton::Button0;
    case BTN_1: return Input::MouseButton::Button1;
    case BTN_2: return Input::MouseButton::Button2;
    case BTN_3: return Input::MouseButton::Button3;
    case BTN_4: return Input::MouseButton::Button4;
    case BTN_5: return Input::MouseButton::Button5;
    default: return Input::MouseButton::Left;
    }
    // clang-format on
}

constexpr Input::KeyState LinuxPlatformImpl::getKeyStateFromCode(uint32_t state) {
    // clang-format off
    switch (state) {
    case WL_KEYBOARD_KEY_STATE_PRESSED: return Input::KeyState::Pressed;
    case WL_KEYBOARD_KEY_STATE_RELEASED: return Input::KeyState::Released;
    case WL_KEYBOARD_KEY_STATE_REPEATED: return Input::KeyState::Held;
    default: return Input::KeyState::None;
    }
    // clang-format on
}

} // namespace dirk::Platform::Linux

#endif
