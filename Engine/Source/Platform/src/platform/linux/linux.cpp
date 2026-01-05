#ifdef PLATFORM_LINUX

#include "platform/linux/linux.hpp"
#include "core.hpp"
#include "input/keys.hpp"
#include "platform/linux/window.hpp"
#include "platform/monitor.hpp"
#include "platform/platform.hpp"

#include "imgui.h"
#include "linux/input-event-codes.h"
#include "vulkan/vulkan_handles.hpp"
#include "wayland-client-core.h"
#include "wayland-client-protocol.h"
#include "wayland-util.h"
#include "xkbcommon/xkbcommon-keysyms.h"
#include "xkbcommon/xkbcommon.h"

#include <cfloat>
#include <cstdint>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>

#define POINTER_SCROLL_SCALE .1f

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

    xkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    check(xkbContext);

    wl_display_roundtrip(display);

    check(compositor && xdgWmBase && dataDeviceManager);
}

LinuxPlatformImpl::~LinuxPlatformImpl() {
    xkb_state_unref(xkbState);
    xkb_keymap_unref(xkbKeymap);
    xkb_context_unref(xkbContext);

    wl_keyboard_destroy(keyboard);
    wl_pointer_destroy(pointer);
    wl_seat_destroy(seat);

    xdg_wm_base_destroy(xdgWmBase);
    wl_compositor_destroy(compositor);
    wl_registry_destroy(registry);
    wl_display_disconnect(display);
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

vk::SurfaceKHR LinuxPlatformImpl::createTempSurface(vk::Instance instance) {
    vk::SurfaceKHR vkSurface;
    auto wlSurface = wl_compositor_create_surface(compositor);

    vk::WaylandSurfaceCreateInfoKHR createInfo;
    createInfo.display = display;
    createInfo.surface = wlSurface;

    vk::detail::DispatchLoaderDynamic dispatcher(instance, vkGetInstanceProcAddr);
    auto err = instance.createWaylandSurfaceKHR(&createInfo, nullptr, &vkSurface, dispatcher);
    if (err != vk::Result::eSuccess)
        DIRK_LOG(LogWayland, FATAL, "received error code {} while attempting to create vulkan surface for wayland surface", (int) err)
    check(vkSurface);

    free(wlSurface);
    wlSurface = nullptr;
    return vkSurface;
}

std::string_view LinuxPlatformImpl::getClipboardText() {
    // TODO: clipboard
    DIRK_LOG(LogWayland, ERROR, "clipboard not implemented");
    return "";
}

void LinuxPlatformImpl::setClipboardText(const std::string& text) {
    // TODO: clipboard
    DIRK_LOG(LogWayland, ERROR, "clipboard not implemented");
}

void LinuxPlatformImpl::beginDrag(LinuxWindowImpl& window, glm::vec2 offset) {
    auto* bd = platform.getBackendData();

    check(toplevelDragManager && dataDeviceManager && seat);

    // Don't start a new drag if one is already in progress
    if (dragInProgress) {
        dragFinished = true;
        return;
    }

    // Need a valid pointer serial from a button press
    if (pointerSerial == 0) {
        DIRK_LOG(LogWayland, ERROR, "requires pointer serial to initiate drag");
        dragFinished = true;
        return;
    }

    // Create data device if not already done
    if (!dataDevice) {
        dataDevice = wl_data_device_manager_get_data_device(dataDeviceManager, seat);
        static const struct wl_data_device_listener dataDeviceListener = {
            .data_offer = wl_DataDeviceOffer,
            .enter = wl_DataDeviceEnter,
            .leave = wl_DataDeviceLeave,
            .motion = wl_DataDeviceMotion,
            .drop = wl_DataDeviceDrop,
            .selection = wl_DataDeviceSelection,
        };
        wl_data_device_add_listener(dataDevice, &dataDeviceListener, this);
    }

    // Create a data source for the drag
    activeDataSource = wl_data_device_manager_create_data_source(dataDeviceManager);
    check(activeDataSource);
    static const struct wl_data_source_listener dataSourceListener = {
        .target = [](void*, struct wl_data_source*, const char*) {},
        .send = [](void*, struct wl_data_source*, const char*, int32_t fd) { close(fd); },
        .cancelled = wl_DataSourceCancelled,
        .dnd_drop_performed = wl_DataSourceDndDropPerformed,
        .dnd_finished = wl_DataSourceDndFinished,
        .action = [](void*, struct wl_data_source*, uint32_t) {}
    };
    wl_data_source_add_listener(activeDataSource, &dataSourceListener, this);

    // Offer MIME types (required even though we don't transfer data)
    wl_data_source_offer(activeDataSource, "application/x-imgui-viewport");
    // rootwindow-drop allows dropping on the desktop/root window in GNOME
    // This prevents the drag from being cancelled when the cursor leaves all surfaces
    wl_data_source_offer(activeDataSource, "application/x-rootwindow-drop");

    // Set allowed DnD actions - MOVE is most appropriate for window dragging
    wl_data_source_set_actions(activeDataSource, WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE);

    // Create the toplevel drag object (but don't attach yet - Chromium does attach after start_drag)
    activeToplevelDrag = xdg_toplevel_drag_manager_v1_get_xdg_toplevel_drag(toplevelDragManager, activeDataSource);
    check(activeToplevelDrag);

    // Start the drag FIRST - origin surface is where the drag starts from
    // The icon surface is nullptr (no drag icon)
    wl_data_device_start_drag(dataDevice, activeDataSource, pointerSurface, nullptr, pointerSerial);

    // Now attach the toplevel to the drag AFTER starting
    // This matches Chromium's implementation order
    // The offset is relative to the toplevel's geometry (surface-local coordinates)
    xdg_toplevel_drag_v1_attach(activeToplevelDrag, window.getToplevel(), offset.x, offset.y);
    draggedWindow = &window;
    dragOffset = offset;

    // Flush to ensure the commands are sent immediately
    wl_display_flush(display);

    dragInProgress = true;
    dragFinished = false;
}

void LinuxPlatformImpl::updateDraggedWindowPos(glm::vec2 globalMousePos) {
    if (!dragInProgress || !draggedWindow)
        return;

    // Calculate window position from mouse position and drag offset
    // Update tracked position so GetWindowPos returns correct value
    draggedWindow->pos = globalMousePos - dragOffset;
}

void LinuxPlatformImpl::cleanupDrag() {
    // Clean up the data offer
    wl_data_offer_destroy(currentDataOffer);
    currentDataOffer = nullptr;

    // Clean up after successful drop
    xdg_toplevel_drag_v1_destroy(activeToplevelDrag);
    activeToplevelDrag = nullptr;

    wl_data_source_destroy(activeDataSource);
    activeDataSource = nullptr;

    draggedWindow = nullptr;
    dragEnterSurface = nullptr;
    dragInProgress = false;
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
            .name = [](void*, struct wl_seat*, const char* name) {},
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
            .description = wl_OutputHandleDescription,
        };
        wl_output_add_listener(output, &outputListener, &platform->platform.createMonitor(output));
    } else if (strcmp(interface, wl_data_device_manager_interface.name) == 0) {
        platform->dataDeviceManager = static_cast<wl_data_device_manager*>(wl_registry_bind(registry, name, &wl_data_device_manager_interface, version));
    } else if (strcmp(interface, xdg_toplevel_drag_manager_v1_interface.name) == 0) {
        platform->toplevelDragManager = static_cast<xdg_toplevel_drag_manager_v1*>(wl_registry_bind(registry, name, &xdg_toplevel_drag_manager_v1_interface, version));
    }
}

void LinuxPlatformImpl::wl_SeatCapabilities(void* data, wl_seat* seat, uint32_t caps) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);

    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !platform->pointer) {
        platform->pointer = wl_seat_get_pointer(seat);

        static const wl_pointer_listener pointerListener = {
            .enter = wl_PointerEnter,
            .leave = wl_PointerLeave,
            .motion = wl_PointerMotion,
            .button = wl_PointerButton,
            .axis = wl_PointerAxis,
            .frame = [](void*, struct wl_pointer*) {},
            .axis_source = [](void*, struct wl_pointer*, uint32_t) {},
            .axis_stop = [](void*, struct wl_pointer*, uint32_t, uint32_t) {},
            .axis_discrete = [](void*, struct wl_pointer*, uint32_t, int32_t) {},
            .axis_value120 = [](void*, struct wl_pointer*, uint32_t, int32_t) {},
            .axis_relative_direction = [](void*, struct wl_pointer*, uint32_t, uint32_t) {},
        };
        wl_pointer_add_listener(platform->pointer, &pointerListener, platform);
    }

    if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !platform->keyboard) {
        platform->keyboard = wl_seat_get_keyboard(seat);

        static const wl_keyboard_listener keyboardListener = {
            .keymap = wl_KeyboardKeymap,
            .enter = wl_KeyboardEnter,
            .leave = wl_KeyboardLeave,
            .key = wl_KeyboardKey,
            .modifiers = wl_KeyboardModifiers,
            .repeat_info = [](void* data, struct wl_keyboard* keyboard, int32_t rate, int32_t delay) {},
        };
        wl_keyboard_add_listener(platform->keyboard, &keyboardListener, platform);
    }

    check(platform->dataDeviceManager && !platform->dataDevice);
    platform->dataDevice = wl_data_device_manager_get_data_device(platform->dataDeviceManager, seat);
}

void LinuxPlatformImpl::wl_OutputHandleGeometry(void* data, struct wl_output* output, int32_t x, int32_t y, int32_t physicalWidth, int32_t physicalHeight, int32_t subpixel, const char* make, const char* model, int32_t transform) {
    auto* monitor = static_cast<Monitor*>(data);
    check(monitor);
    check(monitor->getPlatformHandle() == output);
    monitor->setPosition({ x, y });
    monitor->setMake(make);
    monitor->setModel(model);
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

    DIRK_LOG(LogPlatform, INFO, "found new monitor: \n\tname: {}\n\tmake: {}\n\tmodel: {}\n\tdescription: {}", monitor->getName(), monitor->getMake(), monitor->getModel(), monitor->getDescription())
}

void LinuxPlatformImpl::wl_OutputHandleName(void* data, struct wl_output* output, const char* name) {
    auto* monitor = static_cast<Monitor*>(data);
    check(monitor);
    check(monitor->getPlatformHandle() == output);
    monitor->setName(name);
}

void LinuxPlatformImpl::wl_OutputHandleDescription(void* data, struct wl_output* output, const char* description) {
    auto* monitor = static_cast<Monitor*>(data);
    check(monitor);
    check(monitor->getPlatformHandle() == output);
    monitor->setDescription(description);
}

void LinuxPlatformImpl::wl_PointerEnter(void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface, wl_fixed_t x, wl_fixed_t y) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
    check(platform->pointer == pointer);
    auto viewport = ImGui::FindViewportByPlatformHandle(surface);
    check(viewport);

    platform->pointerSurface = surface;

    auto posX = static_cast<float>(wl_fixed_to_double(x));
    auto posY = static_cast<float>(wl_fixed_to_double(y));
    platform->platform.cursorPosCallback(viewport, { posX, posY });
}

void LinuxPlatformImpl::wl_PointerLeave(void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
    check(platform->pointer == pointer);
    auto viewport = ImGui::FindViewportByPlatformHandle(surface);
    check(viewport);

    platform->pointerSurface = nullptr;
    platform->platform.cursorPosCallback(viewport, { -FLT_MAX, -FLT_MAX });
}

void LinuxPlatformImpl::wl_PointerMotion(void* data, wl_pointer* pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
    check(platform->pointer == pointer);
    auto viewport = ImGui::FindViewportByPlatformHandle(platform->pointerSurface);
    check(viewport);

    glm::vec2 pos = {
        static_cast<float>(wl_fixed_to_double(x)),
        static_cast<float>(wl_fixed_to_double(y))
    };

    platform->platform.cursorPosCallback(viewport, pos);

    auto* bd = platform->platform.getBackendData();
    ImGuiIO& io = ImGui::GetIO(bd->context);
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        pos.x += viewport->Pos.x;
        pos.y += viewport->Pos.y;
    }

    platform->updateDraggedWindowPos(pos);
}

void LinuxPlatformImpl::wl_PointerButton(void* data, wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t inButton, uint32_t inState) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
    check(platform->pointer == pointer);
    auto viewport = ImGui::FindViewportByPlatformHandle(platform->pointerSurface);
    check(viewport);

    Input::KeyState state = getKeyStateFromCode(inState);
    Input::MouseButton button = getMouseFromCode(inButton);

    if (state == Input::KeyState::Pressed && serial != 0) {
        platform->pointerSerial = serial;
        platform->dragFinished = false; // Reset so we can try a new drag with this serial
    }

    platform->platform.mouseButtonCallback(viewport, button, state);
}

void LinuxPlatformImpl::wl_PointerAxis(void* data, wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
    check(platform->pointer == pointer);
    auto viewport = ImGui::FindViewportByPlatformHandle(platform->pointerSurface);
    check(viewport);

    double delta = wl_fixed_to_double(value);
    if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
        platform->platform.mouseScrollCallback(viewport, { 0.f, -delta * POINTER_SCROLL_SCALE });
    } else {
        platform->platform.mouseScrollCallback(viewport, { delta * POINTER_SCROLL_SCALE, 0.f });
    }
}

void LinuxPlatformImpl::wl_KeyboardKeymap(void* data, wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);

    check(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1);
    char* keymapStr = static_cast<char*>(mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0));
    check(keymapStr != MAP_FAILED);

    if (platform->xkbKeymap) {
        xkb_keymap_unref(platform->xkbKeymap);
        platform->xkbKeymap = nullptr;
    }
    platform->xkbKeymap = xkb_keymap_new_from_string(platform->xkbContext, keymapStr, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(static_cast<void*>(keymapStr), size);
    close(fd);

    if (platform->xkbState) {
        xkb_state_unref(platform->xkbState);
        platform->xkbState = nullptr;
    }
    platform->xkbState = xkb_state_new(platform->xkbKeymap);
}

void LinuxPlatformImpl::wl_KeyboardEnter(void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface, wl_array* keys) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
    check(platform->keyboard == keyboard);
    auto viewport = ImGui::FindViewportByPlatformHandle(surface);
    check(viewport);

    platform->keyboardSerial = serial;
    platform->keyboardSurface = surface;

    platform->platform.focusWindowCallback(viewport, true);
}

void LinuxPlatformImpl::wl_KeyboardLeave(void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
    check(platform->keyboard == keyboard);
    auto viewport = ImGui::FindViewportByPlatformHandle(surface);
    check(viewport);

    platform->keyboardSurface = nullptr;

    platform->platform.focusWindowCallback(viewport, false);
}

void LinuxPlatformImpl::wl_KeyboardKey(void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t inKey, uint32_t inState) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
    check(platform->keyboard == keyboard);
    check(platform->xkbState);
    auto viewport = ImGui::FindViewportByPlatformHandle(platform->keyboardSurface);

    uint32_t keycode = inKey + 8;

    xkb_keysym_t sym = xkb_state_key_get_one_sym(platform->xkbState, keycode);

    Input::KeyState state = getKeyStateFromCode(inState);
    Input::Key key = getKeyFromSym(sym);

    if (key == Input::Key::Unknown)
        return;

    platform->platform.keyCallback(viewport, key, state);

    if (state == Input::KeyState::Pressed) {
        uint32_t unicode = xkb_state_key_get_utf32(platform->xkbState, keycode);
        platform->platform.charCallback(viewport, unicode);
    }
}

void LinuxPlatformImpl::wl_KeyboardModifiers(void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t depressed, uint32_t latched, uint32_t locked, uint32_t group) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
    check(platform->keyboard == keyboard);
    check(platform->xkbState);
    xkb_state_update_mask(platform->xkbState, depressed, latched, locked, 0, 0, group);
}

void LinuxPlatformImpl::wl_DataDeviceOffer(void* data, wl_data_device* dataDevice, wl_data_offer* offer) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);

    check(offer);

    // During our own drag, we receive a new offer for each enter.
    // Track the most recent offer so we can clean it up properly.
    if (platform->dragInProgress) {
        if (platform->currentDataOffer && platform->currentDataOffer != offer) {
            wl_data_offer_destroy(platform->currentDataOffer);
        }
        platform->currentDataOffer = offer;
        return;
    }

    // When not dragging, this is likely a selection/clipboard offer.
    // We don't need to track these - they're managed by the compositor.
    // Don't destroy them either - the selection event will reference them.
}

void LinuxPlatformImpl::wl_DataDeviceEnter(void* data, wl_data_device* dataDevice, uint32_t serial, wl_surface* surface, wl_fixed_t x, wl_fixed_t y, wl_data_offer* offer) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);

    // Find viewport for this surface
    ImGuiViewport* enteringViewport = ImGui::FindViewportByPlatformHandle(surface);
    check(enteringViewport);
    ImGuiViewportPlatformData* enteringVd = (ImGuiViewportPlatformData*) enteringViewport->PlatformUserData;
    check(enteringVd);

    glm::vec2 localPos = { wl_fixed_to_double(x), wl_fixed_to_double(y) };

    // Track the surface and position
    platform->dragEnterSurface = surface;
    platform->dragCursorPos = localPos;

    // During our own drag, accept the offer to allow drop
    check(offer);
    if (platform->dragInProgress) {
        // Track this offer - each surface enter gives us a new one
        if (platform->currentDataOffer && platform->currentDataOffer != offer) {
            // Clean up old offer before tracking new one
            wl_data_offer_destroy(platform->currentDataOffer);
        }
        platform->currentDataOffer = offer;

        // Accept the offer to indicate we're a valid drop target
        wl_data_offer_accept(offer, serial, "application/x-imgui-viewport");
        wl_data_offer_set_actions(offer, WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE, WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE);

        // Immediately update ImGui mouse position and viewport on enter
        // This ensures dock previews appear as soon as we enter a target
        ImGuiIO& io = ImGui::GetIO();
        // Tell ImGui which viewport the mouse is now over
        io.AddMouseViewportEvent(enteringViewport->ID);
        glm::vec2 globalPos = glm::vec2{ enteringViewport->Pos.x, enteringViewport->Pos.y } + localPos;
        io.AddMousePosEvent(globalPos.x, globalPos.y);
    }
}

void LinuxPlatformImpl::wl_DataDeviceLeave(void* data, wl_data_device* dataDevice) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
    platform->dragEnterSurface = nullptr;
}

void LinuxPlatformImpl::wl_DataDeviceMotion(void* data, wl_data_device* dataDevice, uint32_t time, wl_fixed_t x, wl_fixed_t y) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);

    platform->dragCursorPos = { wl_fixed_to_double(x), wl_fixed_to_double(y) };

    if (!platform->dragInProgress || !platform->dragEnterSurface)
        return;

    // During drag, update ImGui's mouse position based on the surface we're over
    // The coordinates are relative to the surface we entered
    // Find the viewport for this surface
    ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(platform->dragEnterSurface);
    check(viewport);

    ImGuiIO& io = ImGui::GetIO();

    // Check if we're over the viewport being dragged
    ImGuiViewport* draggedViewport = ImGui::FindViewportByPlatformHandle(platform->draggedWindow->getPlatformHandle());

    // Check if this viewport has a reliable position (main viewport is at 0,0)
    bool isMainViewport = (viewport->Flags & ImGuiViewportFlags_IsPlatformWindow) == 0;

    // Tell ImGui which viewport the mouse is over during drag
    io.AddMouseViewportEvent(viewport->ID);

    if (draggedViewport == viewport) { // over dragged viewport
        // When cursor is over the dragged viewport itself, we can't use its
        // position to calculate global coords (feedback loop). Instead, use
        // the dragged viewport's last known position plus local cursor pos.
        // This lets ImGui track the drag even when over the dragged window.

        glm::vec2 globalPos = glm::vec2{ draggedViewport->Pos.x, draggedViewport->Pos.y } + platform->dragCursorPos;
        io.AddMousePosEvent(globalPos.x, globalPos.y);
        // Don't update dragged viewport position here - would cause feedback loop
    } else if (isMainViewport) {
        // We're over the main viewport which has reliable position (0,0)
        glm::vec2 globalPos = glm::vec2{ viewport->Pos.x, viewport->Pos.y } + platform->dragCursorPos;
        io.AddMousePosEvent(globalPos.x, globalPos.y);

        // Update the dragged viewport's tracked position - this is reliable
        // because we're calculating from a known reference point
        platform->updateDraggedWindowPos(globalPos);
    } else {
        // Over another secondary viewport - use tracked position from viewport data
        glm::vec2 viewportPos = { viewport->Pos.x, viewport->Pos.y };
        /*
        ImGui_ImplWayland_ViewportData* target_vd = (ImGui_ImplWayland_ViewportData*) viewport->PlatformUserData;
        float vp_x = target_vd ? target_vd->PosX : viewport->Pos.x;
        float vp_y = target_vd ? target_vd->PosY : viewport->Pos.y;
        */
        glm::vec2 globalPos = viewportPos + platform->dragCursorPos;
        io.AddMousePosEvent(globalPos.x, globalPos.y);
    }
}

void LinuxPlatformImpl::wl_DataDeviceDrop(void* data, wl_data_device* dataDevice) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);

    // When we receive a drop event, we need to acknowledge it by calling finish()
    // on the current data offer. This signals to the source that the drop was
    // accepted, which triggers dnd_finished instead of cancelled.
    if (platform->currentDataOffer) {
        wl_data_offer_finish(platform->currentDataOffer);
        // Don't destroy the offer here - it will be cleaned up in dnd_finished/cancelled
    }
}

void LinuxPlatformImpl::wl_DataDeviceSelection(void* data, wl_data_device* dataDevice, wl_data_offer* offer) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);

    // TODO: clipboard support
    // Selection/clipboard offer - we don't handle clipboard, so ignore this.
    // The offer is managed by the compositor for clipboard operations.
}

void LinuxPlatformImpl::wl_DataSourceCancelled(void* data, struct wl_data_source* dataSource) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);

    // For xdg_toplevel_drag, "cancelled" often means the drag ended without a valid
    // drop target, but the window was still moved successfully by the compositor.
    // This is especially common on GNOME when dropping in empty space or when the
    // pointer leaves all surfaces briefly during the drag.
    bool wasToplevelDrag = platform->activeToplevelDrag != nullptr;

    if (wasToplevelDrag) {
        // For toplevel drags, "cancelled" is often a successful window move
        // The compositor has already positioned the window where we want it
    }

    check(dataSource == platform->activeDataSource);
    platform->cleanupDrag();
    platform->dragFinished = true;
}

void LinuxPlatformImpl::wl_DataSourceDndDropPerformed(void* data, struct wl_data_source* dataSource) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);

    // During DnD, the pointer is grabbed and button release events don't come through
    // wl_pointer - we must notify ImGui of the button release here so it can process
    // the drop action (e.g., dock a window).
    if (platform->dragInProgress) {
        ImGuiIO& io = ImGui::GetIO();

        // Ensure ImGui has the final mouse position and viewport before we release
        // Use the last known drag cursor position relative to the enter surface
        // Use viewport->Pos instead of vd->PosX/PosY since Wayland doesn't provide
        // reliable position info for secondary viewports
        if (platform->dragEnterSurface) {
            ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(platform->dragEnterSurface);
            check(viewport);
            // CRITICAL: Tell ImGui which viewport we're over for docking to work
            io.AddMouseViewportEvent(viewport->ID);
            glm::vec2 globalPos = glm::vec2{ viewport->Pos.x, viewport->Pos.y } - platform->dragCursorPos;
            io.AddMousePosEvent(globalPos.x, globalPos.y);
        }

        io.AddMouseButtonEvent(0, false); // Release left mouse button
    }

    // Drag finished - the toplevel's final position is determined by the compositor
    platform->dragFinished = true;
}

void LinuxPlatformImpl::wl_DataSourceDndFinished(void* data, struct wl_data_source* dataSource) {
    auto* platform = static_cast<LinuxPlatformImpl*>(data);
    check(dataSource == platform->activeDataSource);
    platform->cleanupDrag();
}

constexpr Input::Key LinuxPlatformImpl::getKeyFromSym(xkb_keysym_t sym) {
    // clang-format off
    switch (sym)
    {
    // Letters
    case XKB_KEY_a: case XKB_KEY_A: return Input::Key::A;
    case XKB_KEY_b: case XKB_KEY_B: return Input::Key::B;
    case XKB_KEY_c: case XKB_KEY_C: return Input::Key::C;
    case XKB_KEY_d: case XKB_KEY_D: return Input::Key::D;
    case XKB_KEY_e: case XKB_KEY_E: return Input::Key::E;
    case XKB_KEY_f: case XKB_KEY_F: return Input::Key::F;
    case XKB_KEY_g: case XKB_KEY_G: return Input::Key::G;
    case XKB_KEY_h: case XKB_KEY_H: return Input::Key::H;
    case XKB_KEY_i: case XKB_KEY_I: return Input::Key::I;
    case XKB_KEY_j: case XKB_KEY_J: return Input::Key::J;
    case XKB_KEY_k: case XKB_KEY_K: return Input::Key::K;
    case XKB_KEY_l: case XKB_KEY_L: return Input::Key::L;
    case XKB_KEY_m: case XKB_KEY_M: return Input::Key::M;
    case XKB_KEY_n: case XKB_KEY_N: return Input::Key::N;
    case XKB_KEY_o: case XKB_KEY_O: return Input::Key::O;
    case XKB_KEY_p: case XKB_KEY_P: return Input::Key::P;
    case XKB_KEY_q: case XKB_KEY_Q: return Input::Key::Q;
    case XKB_KEY_r: case XKB_KEY_R: return Input::Key::R;
    case XKB_KEY_s: case XKB_KEY_S: return Input::Key::S;
    case XKB_KEY_t: case XKB_KEY_T: return Input::Key::T;
    case XKB_KEY_u: case XKB_KEY_U: return Input::Key::U;
    case XKB_KEY_v: case XKB_KEY_V: return Input::Key::V;
    case XKB_KEY_w: case XKB_KEY_W: return Input::Key::W;
    case XKB_KEY_x: case XKB_KEY_X: return Input::Key::X;
    case XKB_KEY_y: case XKB_KEY_Y: return Input::Key::Y;
    case XKB_KEY_z: case XKB_KEY_Z: return Input::Key::Z;
        
    // Numbers
    case XKB_KEY_1: case XKB_KEY_exclam: return Input::Key::D1;
    case XKB_KEY_2: case XKB_KEY_at: return Input::Key::D2;
    case XKB_KEY_3: case XKB_KEY_numbersign: return Input::Key::D3;
    case XKB_KEY_4: case XKB_KEY_dollar: return Input::Key::D4;
    case XKB_KEY_5: case XKB_KEY_percent: return Input::Key::D5;
    case XKB_KEY_6: case XKB_KEY_asciicircum: return Input::Key::D6;
    case XKB_KEY_7: case XKB_KEY_ampersand: return Input::Key::D7;
    case XKB_KEY_8: case XKB_KEY_asterisk: return Input::Key::D8;
    case XKB_KEY_9: case XKB_KEY_parenleft: return Input::Key::D9;
    case XKB_KEY_0: case XKB_KEY_parenright: return Input::Key::D0;
        
    // Special keys
    case XKB_KEY_Return: return Input::Key::Enter;
    case XKB_KEY_Escape: return Input::Key::Escape;
    case XKB_KEY_BackSpace: return Input::Key::Backspace;
    case XKB_KEY_Tab: case XKB_KEY_ISO_Left_Tab: return Input::Key::Tab;
    case XKB_KEY_space: return Input::Key::Space;
    case XKB_KEY_minus: case XKB_KEY_underscore: return Input::Key::Minus;
    case XKB_KEY_equal: case XKB_KEY_plus: return Input::Key::Equal;
    case XKB_KEY_bracketleft: case XKB_KEY_braceleft: return Input::Key::LeftBracket;
    case XKB_KEY_bracketright: case XKB_KEY_braceright: return Input::Key::RightBracket;
    case XKB_KEY_backslash: case XKB_KEY_bar: return Input::Key::Backslash;
    case XKB_KEY_semicolon: case XKB_KEY_colon: return Input::Key::Semicolon;
    case XKB_KEY_apostrophe: case XKB_KEY_quotedbl: return Input::Key::Apostrophe;
    case XKB_KEY_grave: case XKB_KEY_asciitilde: return Input::Key::GraveAccent;
    case XKB_KEY_comma: case XKB_KEY_less: return Input::Key::Comma;
    case XKB_KEY_period: case XKB_KEY_greater: return Input::Key::Period;
    case XKB_KEY_slash: case XKB_KEY_question: return Input::Key::Slash;
    case XKB_KEY_Caps_Lock: return Input::Key::CapsLock;
        
    // Function keys
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
        
    // System keys
    case XKB_KEY_Print: case XKB_KEY_Sys_Req: return Input::Key::PrintScreen;
    case XKB_KEY_Scroll_Lock: return Input::Key::ScrollLock;
    case XKB_KEY_Pause: case XKB_KEY_Break: return Input::Key::Pause;
    case XKB_KEY_Insert: return Input::Key::Insert;
    case XKB_KEY_Home: return Input::Key::Home;
    case XKB_KEY_Page_Up: return Input::Key::PageUp;
    case XKB_KEY_Delete: return Input::Key::Delete;
    case XKB_KEY_End: return Input::Key::End;
    case XKB_KEY_Page_Down: return Input::Key::PageDown;
        
    // Arrow keys
    case XKB_KEY_Right: return Input::Key::Right;
    case XKB_KEY_Left: return Input::Key::Left;
    case XKB_KEY_Down: return Input::Key::Down;
    case XKB_KEY_Up: return Input::Key::Up;
        
    // Numpad
    case XKB_KEY_Num_Lock: return Input::Key::NumLock;
    case XKB_KEY_KP_Divide: return Input::Key::KPDivide;
    case XKB_KEY_KP_Multiply: return Input::Key::KPMultiply;
    case XKB_KEY_KP_Subtract: return Input::Key::KPMinus;
    case XKB_KEY_KP_Add: return Input::Key::KPPlus;
    case XKB_KEY_KP_Enter: return Input::Key::KPEnter;
    case XKB_KEY_KP_1: case XKB_KEY_KP_End: return Input::Key::KP1;
    case XKB_KEY_KP_2: case XKB_KEY_KP_Down: return Input::Key::KP2;
    case XKB_KEY_KP_3: case XKB_KEY_KP_Page_Down: return Input::Key::KP3;
    case XKB_KEY_KP_4: case XKB_KEY_KP_Left: return Input::Key::KP4;
    case XKB_KEY_KP_5: case XKB_KEY_KP_Begin: return Input::Key::KP5;
    case XKB_KEY_KP_6: case XKB_KEY_KP_Right: return Input::Key::KP6;
    case XKB_KEY_KP_7: case XKB_KEY_KP_Home: return Input::Key::KP7;
    case XKB_KEY_KP_8: case XKB_KEY_KP_Up: return Input::Key::KP8;
    case XKB_KEY_KP_9: case XKB_KEY_KP_Page_Up: return Input::Key::KP9;
    case XKB_KEY_KP_0: case XKB_KEY_KP_Insert: return Input::Key::KP0;
    case XKB_KEY_KP_Decimal: case XKB_KEY_KP_Delete: return Input::Key::KPPeriod;
        
    // Modifiers
    case XKB_KEY_Control_L: return Input::Key::LeftCtrl;
    case XKB_KEY_Shift_L: return Input::Key::LeftShift;
    case XKB_KEY_Alt_L: return Input::Key::LeftAlt;
    case XKB_KEY_Super_L: case XKB_KEY_Meta_L: return Input::Key::LeftSuper;
    case XKB_KEY_Control_R: return Input::Key::RightCtrl;
    case XKB_KEY_Shift_R: return Input::Key::RightShift;
    case XKB_KEY_Alt_R: case XKB_KEY_ISO_Level3_Shift: return Input::Key::RightAlt;
    case XKB_KEY_Super_R: case XKB_KEY_Meta_R: return Input::Key::RightSuper;
        
    // Media keys
    case XKB_KEY_XF86AudioMute: return Input::Key::Mute;
    case XKB_KEY_XF86AudioRaiseVolume: return Input::Key::VolumeUp;
    case XKB_KEY_XF86AudioLowerVolume: return Input::Key::VolumeDown;
    default:
        DIRK_LOG(LogWayland, ERROR, "keycode {} is unknown", sym)
        return Input::Key::Unknown;
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
    default:
        DIRK_LOG(LogWayland, ERROR, "mouse button {} is unknown", button)
        return Input::MouseButton::Left;
    }
    // clang-format on
}

constexpr Input::KeyState LinuxPlatformImpl::getKeyStateFromCode(uint32_t state) {
    // clang-format off
    switch (state) {
    case WL_KEYBOARD_KEY_STATE_PRESSED: return Input::KeyState::Pressed;
    case WL_KEYBOARD_KEY_STATE_RELEASED: return Input::KeyState::Released;
    case WL_KEYBOARD_KEY_STATE_REPEATED: return Input::KeyState::Held;
    default:
        DIRK_LOG(LogWayland, ERROR, "key state {} is unknown", state)
        return Input::KeyState::None;
    }
    // clang-format on
}

} // namespace dirk::Platform::Linux

#endif
