#include "platform/platform.hpp"

#include "imgui.h"

#include <memory>

namespace dirk::Platform {

Platform::Platform(const PlatformCreateInfo& createInfo) {
    // TODO: create first simple window to be used in renderer init (for surface format detection and stuff)
    std::shared_ptr<Window> window;

    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");

    ImGui_Data* bd = IM_NEW(ImGui_Data)();

    io.BackendPlatformUserData = (void*) bd;
    io.BackendPlatformName = bd->platformName.data();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;    // We can create multi-viewports on the Platform side (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can call io.AddMouseViewportEvent() with correct data (optional)

    bd->context = ImGui::GetCurrentContext();

    bd->window = window;

    contextMap[window] = bd->context;

    // platform support
    ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
    platformIO.Platform_CreateWindow = CreateWindow;
    platformIO.Platform_DestroyWindow = DestroyWindow;
    platformIO.Platform_ShowWindow = ShowWindow;
    platformIO.Platform_SetWindowPos = SetWindowPos;
    platformIO.Platform_GetWindowPos = GetWindowPos;
    platformIO.Platform_SetWindowSize = SetWindowSize;
    platformIO.Platform_GetWindowSize = GetWindowSize;
    platformIO.Platform_GetWindowFramebufferScale = GetWindowFramebufferScale;
    platformIO.Platform_SetWindowFocus = SetWindowFocus;
    platformIO.Platform_GetWindowFocus = GetWindowFocus;
    platformIO.Platform_GetWindowMinimized = GetWindowMinimized;
    platformIO.Platform_SetWindowTitle = SetWindowTitle;
    platformIO.Platform_CreateVkSurface = CreateVkSurface;

    platformIO.Platform_GetClipboardTextFn = nullptr;
    platformIO.Platform_SetClipboardTextFn = nullptr;
    platformIO.Platform_OpenInShellFn = nullptr;

    // TODO: create mouse cursors
    bd->mouseCursors[ImGuiMouseCursor_Arrow] = Cursor{};
    bd->mouseCursors[ImGuiMouseCursor_TextInput] = Cursor{};
    bd->mouseCursors[ImGuiMouseCursor_ResizeNS] = Cursor{};
    bd->mouseCursors[ImGuiMouseCursor_ResizeEW] = Cursor{};
    bd->mouseCursors[ImGuiMouseCursor_Hand] = Cursor{};
    bd->mouseCursors[ImGuiMouseCursor_ResizeAll] = Cursor{};
    bd->mouseCursors[ImGuiMouseCursor_ResizeNESW] = Cursor{};
    bd->mouseCursors[ImGuiMouseCursor_ResizeNWSE] = Cursor{};
    bd->mouseCursors[ImGuiMouseCursor_NotAllowed] = Cursor{};

    // setup main viewport
    ImGui_ViewportData* vd = IM_NEW(ImGui_ViewportData)();
    vd->window = bd->window;
    vd->windowOwned = false;

    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    main_viewport->PlatformUserData = vd;
    main_viewport->PlatformHandle = (void*) bd->window.get();
    IM_UNUSED(main_viewport);
}

Platform::~Platform() {
    ImGui_Data* bd = getBackendData();
    IM_ASSERT(bd != nullptr && "No platform backend to shutdown, or already shutdown?");

    ImGuiIO& io = ImGui::GetIO();
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

    ImGui::DestroyPlatformWindows();

    // TODO: destroy cursors
    /**
    for (ImGuiMouseCursor cursor_n = 0; cursor_n < ImGuiMouseCursor_COUNT; cursor_n++)
        glfwDestroyCursor(bd->mouseCursors[cursor_n]);
    */

    io.BackendPlatformName = nullptr;
    io.BackendPlatformUserData = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos | ImGuiBackendFlags_HasGamepad | ImGuiBackendFlags_PlatformHasViewports | ImGuiBackendFlags_HasMouseHoveredViewport);
    platform_io.ClearPlatformHandlers();
    contextMap.erase(bd->window);
    IM_DELETE(bd);
}

void Platform::tick(float deltaTime) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui_Data* bd = getBackendData();
    check(bd);

    io.DeltaTime = deltaTime;
    bd->mouseIgnoreButtonUp = false;

    auto size = bd->window->getSize();
    io.DisplaySize = ImVec2(size.width, size.height);
    // Apple only
    // auto fbSize = bd->window->getFramebufferSize();
    // io.DisplayFramebufferScale = ImVec2((float) fbSize.width / (float) size.width, (float) fbSize.height / (float) size.height);

    updateMonitors();
    updateMouseData();
    updateMouseCursor();
}

// TODO: implement platform funcs
void Platform::CreateWindow(ImGuiViewport* vp) {}
void Platform::DestroyWindow(ImGuiViewport* vp) {}
void Platform::ShowWindow(ImGuiViewport* vp) {}
void Platform::SetWindowPos(ImGuiViewport* vp, ImVec2 pos) {}
ImVec2 Platform::GetWindowPos(ImGuiViewport* vp) {}
void Platform::SetWindowSize(ImGuiViewport* vp, ImVec2 size) {}
ImVec2 Platform::GetWindowSize(ImGuiViewport* vp) {}
ImVec2 Platform::GetWindowFramebufferScale(ImGuiViewport* vp) {}
void Platform::SetWindowFocus(ImGuiViewport* vp) {}
bool Platform::GetWindowFocus(ImGuiViewport* vp) {}
bool Platform::GetWindowMinimized(ImGuiViewport* vp) {}
void Platform::SetWindowTitle(ImGuiViewport* vp, const char* str) {}
int Platform::CreateVkSurface(ImGuiViewport* vp, ImU64 vk_inst, const void* vk_allocators, ImU64* out_vk_surface) {}

// TODO: implement platfrom event callbacks
void Platform::focusWindowCallback(std::shared_ptr<Window> window, bool focused) {}
void Platform::cursorEnterCallback(std::shared_ptr<Window> window, bool entered) {}
void Platform::cursorPosCallback(std::shared_ptr<Window> window, glm::vec2 pos) {}
void Platform::mouseButtonCallback(std::shared_ptr<Window> window, int button, int action, int mods) {}
void Platform::mouseScrollCallback(std::shared_ptr<Window> window, glm::vec2 offset) {}
void Platform::keyCallback(std::shared_ptr<Window> window, int keycode, int scancode, int action, int mods) {}
void Platform::charCallback(std::shared_ptr<Window> window, unsigned int c) {}

ImGui_Data* Platform::getBackendData() {
    return ImGui::GetCurrentContext() ? (ImGui_Data*) ImGui::GetIO().BackendPlatformUserData : nullptr;
}

ImGui_Data* Platform::getBackendData(std::shared_ptr<Window> window) {
    ImGuiContext* ctx = contextMap.at(window);
    return (ImGui_Data*) ImGui::GetIO(ctx).BackendPlatformUserData;
}

void Platform::updateMonitors() {
    // TODO: update monitors
    /**
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

    int monitors_count = 0;
    GLFWmonitor** glfw_monitors = glfwGetMonitors(&monitors_count);
    if (monitors_count == 0) // Preserve existing monitor list if there are none. Happens on macOS sleeping (#5683)
        return;

    platform_io.Monitors.resize(0);
    for (int n = 0; n < monitors_count; n++) {
        ImGuiPlatformMonitor monitor;
        int x, y;
        glfwGetMonitorPos(glfw_monitors[n], &x, &y);
        const GLFWvidmode* vid_mode = glfwGetVideoMode(glfw_monitors[n]);
        if (vid_mode == nullptr)
            continue; // Failed to get Video mode (e.g. Emscripten does not support this function)
        monitor.MainPos = monitor.WorkPos = ImVec2((float) x, (float) y);
        monitor.MainSize = monitor.WorkSize = ImVec2((float) vid_mode->width, (float) vid_mode->height);
#if GLFW_HAS_MONITOR_WORK_AREA
        int w, h;
        glfwGetMonitorWorkarea(glfw_monitors[n], &x, &y, &w, &h);
        if (w > 0 && h > 0) // Workaround a small GLFW issue reporting zero on monitor changes: https://github.com/glfw/glfw/pull/1761
        {
            monitor.WorkPos = ImVec2((float) x, (float) y);
            monitor.WorkSize = ImVec2((float) w, (float) h);
        }
#endif
        float scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfw_monitors[n]);
        if (scale == 0.0f)
            continue; // Some accessibility applications are declaring virtual monitors with a DPI of 0, see #7902.
        monitor.DpiScale = scale;
        monitor.PlatformHandle = (void*) glfw_monitors[n]; // [...] GLFW doc states: "guaranteed to be valid only until the monitor configuration changes"
        platform_io.Monitors.push_back(monitor);
    }
    */
}

void Platform::updateMouseData() {
    // TODO: update mouse data
    /**
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    ImGuiIO& io = ImGui::GetIO();
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

    ImGuiID mouse_viewport_id = 0;
    const ImVec2 mouse_pos_prev = io.MousePos;
    for (int n = 0; n < platform_io.Viewports.Size; n++) {
        ImGuiViewport* viewport = platform_io.Viewports[n];
        GLFWwindow* window = (GLFWwindow*) viewport->PlatformHandle;

#ifdef EMSCRIPTEN_USE_EMBEDDED_GLFW3
        const bool is_window_focused = true;
#else
        const bool is_window_focused = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;
#endif
        if (is_window_focused) {
            // (Optional) Set OS mouse position from Dear ImGui if requested (rarely used, only when io.ConfigNavMoveSetMousePos is enabled by user)
            // When multi-viewports are enabled, all Dear ImGui positions are same as OS positions.
            if (io.WantSetMousePos)
                glfwSetCursorPos(window, (double) (mouse_pos_prev.x - viewport->Pos.x), (double) (mouse_pos_prev.y - viewport->Pos.y));

            // (Optional) Fallback to provide mouse position when focused (ImGui_ImplGlfw_CursorPosCallback already provides this when hovered or captured)
            if (bd->MouseWindow == nullptr) {
                double mouse_x, mouse_y;
                glfwGetCursorPos(window, &mouse_x, &mouse_y);
                if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                    // Single viewport mode: mouse position in client window coordinates (io.MousePos is (0,0) when the mouse is on the upper-left corner of the app window)
                    // Multi-viewport mode: mouse position in OS absolute coordinates (io.MousePos is (0,0) when the mouse is on the upper-left of the primary monitor)
                    int window_x, window_y;
                    glfwGetWindowPos(window, &window_x, &window_y);
                    mouse_x += window_x;
                    mouse_y += window_y;
                }
                bd->LastValidMousePos = ImVec2((float) mouse_x, (float) mouse_y);
                io.AddMousePosEvent((float) mouse_x, (float) mouse_y);
            }
        }

        // (Optional) When using multiple viewports: call io.AddMouseViewportEvent() with the viewport the OS mouse cursor is hovering.
        // If ImGuiBackendFlags_HasMouseHoveredViewport is not set by the backend, Dear imGui will ignore this field and infer the information using its flawed heuristic.
        // - [X] GLFW >= 3.3 backend ON WINDOWS ONLY does correctly ignore viewports with the _NoInputs flag (since we implement hit via our WndProc hook)
        //       On other platforms we rely on the library fallbacking to its own search when reporting a viewport with _NoInputs flag.
        // - [!] GLFW <= 3.2 backend CANNOT correctly ignore viewports with the _NoInputs flag, and CANNOT reported Hovered Viewport because of mouse capture.
        //       Some backend are not able to handle that correctly. If a backend report an hovered viewport that has the _NoInputs flag (e.g. when dragging a window
        //       for docking, the viewport has the _NoInputs flag in order to allow us to find the viewport under), then Dear ImGui is forced to ignore the value reported
        //       by the backend, and use its flawed heuristic to guess the viewport behind.
        // - [X] GLFW backend correctly reports this regardless of another viewport behind focused and dragged from (we need this to find a useful drag and drop target).
        // FIXME: This is currently only correct on Win32. See what we do below with the WM_NCHITTEST, missing an equivalent for other systems.
        // See https://github.com/glfw/glfw/issues/1236 if you want to help in making this a GLFW feature.
#if GLFW_HAS_MOUSE_PASSTHROUGH
        const bool window_no_input = (viewport->Flags & ImGuiViewportFlags_NoInputs) != 0;
        glfwSetWindowAttrib(window, GLFW_MOUSE_PASSTHROUGH, window_no_input);
#endif
#if GLFW_HAS_MOUSE_PASSTHROUGH || GLFW_HAS_WINDOW_HOVERED
        if (glfwGetWindowAttrib(window, GLFW_HOVERED))
            mouse_viewport_id = viewport->ID;
#else
        // We cannot use bd->MouseWindow maintained from CursorEnter/Leave callbacks, because it is locked to the window capturing mouse.
#endif
    }

    if (io.BackendFlags & ImGuiBackendFlags_HasMouseHoveredViewport)
        io.AddMouseViewportEvent(mouse_viewport_id);

    */
}

void Platform::updateMouseCursor() {
    // TODO: update mouse cursor
    /**
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) || glfwGetInputMode(bd->Window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        return;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    for (int n = 0; n < platform_io.Viewports.Size; n++) {
        GLFWwindow* window = (GLFWwindow*) platform_io.Viewports[n]->PlatformHandle;
        if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor) {
            // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        } else {
            // Show OS mouse cursor
            // FIXME-PLATFORM: Unfocused windows seems to fail changing the mouse cursor with GLFW 3.2, but 3.3 works here.
            glfwSetCursor(window, bd->MouseCursors[imgui_cursor] ? bd->MouseCursors[imgui_cursor] : bd->MouseCursors[ImGuiMouseCursor_Arrow]);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    */
}

} // namespace dirk::Platform
