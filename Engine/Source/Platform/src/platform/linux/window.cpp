#include "core.hpp"

#ifdef PLATFORM_LINUX

#include "Events/EventManager.hpp"
#include "logging/logging.hpp"
#include "platform/events.hpp"
#include "platform/linux/linux.hpp"
#include "platform/linux/window.hpp"
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

    ImGuiIO& io = ImGui::GetIO();
    bool shouldDrag = io.MouseDown[0] && (linuxPlatform.pointerSerial != 0);

    if (shouldDrag) {
        // Calculate offset using local surface coordinates from the source surface
        // The offset is relative to the toplevel geometry being attached
        auto offset = linuxPlatform.platform.getMouseLocalPos();

        // Start the drag operation - attaching the already-mapped toplevel
        linuxPlatform.beginDrag(*this, offset);
    }
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

    // During a toplevel drag, the compositor controls the window position.
    // Ignore SetWindowPos calls for the dragged toplevel - they come from ImGui
    // trying to sync position, but the compositor is in charge during drag.
    if (linuxPlatform.dragInProgress && linuxPlatform.draggedWindow == this) {
        // Don't update position during drag - the compositor handles it
        return;
    }

    // Ignore obviously bogus positions (INT_MIN converted to float)
    if (pos.y < -1000000.0f || pos.x < -1000000.0f) {
        return;
    }

    // Wayland does not allow clients to set window position directly.
    // However, we can use xdg_toplevel_drag to move existing windows.
    // Per protocol: "Dragging an existing window is similar. The client creates
    // a xdg_toplevel_drag_v1 object and attaches the existing toplevel before
    // starting the drag."
    //
    // Important: Only try once per drag operation. If the drag was cancelled,
    // don't retry until a new button press (new serial).
    bool dominatedByDrag = !linuxPlatform.dragInProgress && !linuxPlatform.dragFinished && xdgToplevel &&
                           linuxPlatform.toplevelDragManager && linuxPlatform.dataDeviceManager && linuxPlatform.seat;

    if (dominatedByDrag) {
        // Calculate offset: use local surface coordinates (relative to toplevel geometry)
        // This matches how Chromium passes offsets to xdg_toplevel_drag_v1_attach
        auto offset = linuxPlatform.platform.getMouseLocalPos();

        linuxPlatform.beginDrag(*this, offset);
    }

    // Track the position that ImGui wants internally so that
    // GetWindowPos can return it and input coordinate transformations work.
    this->pos = inPosition;
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

    bool isFullscreen = false;
    for (uint32_t* state = (uint32_t*) states->data;
         (const char*) state < ((const char*) states->data + states->size);
         state++) {
        if (*state == XDG_TOPLEVEL_STATE_FULLSCREEN)
            isFullscreen = true;
    }

    // Handle fullscreen state changes - unset parent when fullscreen to allow
    // other windows to receive input (important for multi-monitor setups)
    if (window->fullscreen != isFullscreen) {
        window->fullscreen = isFullscreen;

        // Find the viewport to update its position
        ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
        ImGuiViewport* viewport = nullptr;
        for (int i = 0; i < platformIO.Viewports.Size; i++) {
            if (platformIO.Viewports[i]->PlatformHandle == window->getPlatformHandle()) {
                viewport = platformIO.Viewports[i];
                break;
            }
        }

        if (window->fullscreen) {
            // Unset parent so this fullscreen window doesn't block input to others
            xdg_toplevel_set_parent(toplevel, nullptr);

            // Save current position and reset to (0,0) for fullscreen
            // TODO: For multi-monitor, determine which monitor and use its origin
            window->savedPos = window->pos;
            window->pos = { 0, 0 };
        } else {
            // Restore parent relationship when exiting fullscreen
            auto* bd = window->linuxPlatform.getPlatform().getBackendData();
            xdg_toplevel_set_parent(toplevel, static_cast<LinuxWindowImpl*>(bd->mainWindow)->xdgToplevel);

            // Restore saved position
            window->pos = window->savedPos;
        }
        // Notify ImGui that position changed
        check(viewport);
        viewport->PlatformRequestMove = true;
    }

    vk::Extent2D newSize(width, height);
    if (newSize == window->size)
        return;

    window->size = newSize;
    // TODO: as we wait for configuration to finish before returning the constructor,
    // the platform handle is not yet set in the viewport variable. it thus cannot find it
    auto* viewport = ImGui::FindViewportByPlatformHandle(window->wlSurface);
    if (viewport)
        DIRK_DISPATCH_EVENT(WindowResizeEvent, viewport, window->size);
}

void LinuxWindowImpl::xdg_ToplevelClose(void* data, xdg_toplevel* toplevel) {
    auto* window = static_cast<LinuxWindowImpl*>(data);
    auto* viewport = ImGui::FindViewportByPlatformHandle(window->wlSurface);
    check(viewport);
    DIRK_DISPATCH_EVENT(WindowCloseEvent, viewport);
}

} // namespace dirk::Platform::Linux

#endif
