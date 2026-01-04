// dear imgui: Platform Backend for Wayland (*EXPERIMENTAL*)
// This needs to be used along with a Renderer (e.g. DirectX11, OpenGL3, Vulkan..)
// (Info: Wayland is a Linux display compositor protocol implemented by various display servers for graphical user interfaces.)

// Implemented features:
//  [X] Platform: Clipboard support. (TODO)
//  [X] Platform: Mouse support. Can discriminate Mouse/TouchScreen.
//  [X] Platform: Keyboard support. Since 1.87 we are using the io.AddKeyEvent() function. Pass ImGuiKey values to all key functions e.g. ImGui::IsKeyPressed(ImGuiKey_Space).
//  [ ] Platform: Gamepad support. Enabled with 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad'.
//  [X] Platform: Mouse cursor shape and visibility (ImGuiBackendFlags_HasMouseCursors). Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.
//  [ ] Platform: IME support.
//  [X] Platform: Multi-viewport support (multiple windows). Enable with 'io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable'.

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#pragma once
#include "imgui.h"      // IMGUI_IMPL_API
#ifndef IMGUI_DISABLE

union Input_Event;

// Forward declarations of Wayland types
struct wl_display;
struct wl_surface;
struct wl_compositor;
struct xdg_wm_base;
struct xdg_toplevel;
struct wl_seat;
struct wp_fractional_scale_manager_v1;
struct wp_viewporter;
struct wp_cursor_shape_manager_v1;
struct wl_data_device_manager;
struct xdg_toplevel_drag_manager_v1;
struct zxdg_output_manager_v1;
struct wl_output;

struct ImGui_ImplWayland_FrameMetrics
{
    uint32_t DisplayWidth;
    uint32_t DisplayHeight;
    uint32_t BufferWidth;
    uint32_t BufferHeight;
    uint32_t FractionalScale; // Numerator for framebuffer scale (denominator 120)
    uint32_t InputScale;      // Numerator for input coordinate scale (denominator 120)
};

typedef void    *WaylandWSIWindow;
typedef void    (*ImGui_ImplWayland_SetCursorFunc)(WaylandWSIWindow window, ImGuiMouseCursor cursor, bool visible);

// Wayland globals needed for multi-viewport support (creating secondary windows)
struct ImGui_ImplWayland_InitData
{
    // Required: Wayland display connection
    struct wl_display*                      Display;

    // Required: Main window surface (for the main viewport)
    struct wl_surface*                      MainSurface;

    // Required: Main window toplevel (for setting parent on secondary viewports)
    struct xdg_toplevel*                    MainToplevel;

    // Required: Wayland globals for creating windows
    struct wl_compositor*                   Compositor;
    struct xdg_wm_base*                     WmBase;
    struct wl_seat*                         Seat;

    // Optional: For fractional scaling support on secondary viewports
    struct wp_fractional_scale_manager_v1*  FractionalScaleManager;
    struct wp_viewporter*                   Viewporter;

    // Optional: For cursor shape support
    struct wp_cursor_shape_manager_v1*      CursorShapeManager;

    // Optional: For toplevel drag support (detaching windows from docks)
    struct wl_data_device_manager*          DataDeviceManager;
    struct xdg_toplevel_drag_manager_v1*    ToplevelDragManager;

    // Optional: For monitor information (multi-viewport positioning)
    struct zxdg_output_manager_v1*          XdgOutputManager;

    // Main viewport metrics
    ImGui_ImplWayland_FrameMetrics          Metrics;

    // Optional: Cursor callback (for custom cursor handling)
    ImGui_ImplWayland_SetCursorFunc         SetCursorFunc;
    WaylandWSIWindow                        SetCursorFuncUserData;
};

// Legacy configuration data (for single-viewport mode)
struct ImGui_ImplWayland_ConfigurationData {
    WaylandWSIWindow window;
    ImGui_ImplWayland_FrameMetrics metrics;
    ImGui_ImplWayland_SetCursorFunc setCursorFunc;
};

// Follow "Getting Started" link and check examples/ folder to learn about using backends!
// Multi-viewport enabled init (recommended)
IMGUI_IMPL_API bool     ImGui_ImplWayland_InitMultiViewport(const ImGui_ImplWayland_InitData* init_data);
// Legacy single-viewport init
IMGUI_IMPL_API bool     ImGui_ImplWayland_Init(const ImGui_ImplWayland_ConfigurationData *configData);
IMGUI_IMPL_API void     ImGui_ImplWayland_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplWayland_UpdateFrameMetrics(const ImGui_ImplWayland_FrameMetrics *metrics);
IMGUI_IMPL_API void     ImGui_ImplWayland_NewFrame();
IMGUI_IMPL_API bool     ImGui_ImplWayland_ProcessEvent(const Input_Event *event);

// Multi-viewport support: Process event for a specific surface (returns viewport that received the event)
IMGUI_IMPL_API bool     ImGui_ImplWayland_ProcessEventForSurface(const Input_Event *event, struct wl_surface* surface);

// Multi-viewport support: Register/unregister outputs for monitor tracking
// Call these when wl_output objects are added/removed from the registry
IMGUI_IMPL_API void     ImGui_ImplWayland_AddOutput(struct wl_output* output, uint32_t name);
IMGUI_IMPL_API void     ImGui_ImplWayland_RemoveOutput(uint32_t name);

// Get the serial from the most recent pointer enter event (for cursor shape protocol)
IMGUI_IMPL_API uint32_t ImGui_ImplWayland_GetPointerEnterSerial();

#endif // #ifndef IMGUI_DISABLE

