#include "platform/platform.hpp"
#include "imgui.h"
#include <memory>

namespace dirk::Platform {

Platform::Platform(const PlatformCreateInfo& createInfo) {
    // TODO: create first simple window to be used in renderer init (for surface format detection and stuff)
}

void Platform::ImGui_Init() {
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");

    ImGui_ImplDirk_Data* bd = IM_NEW(ImGui_ImplDirk_Data)();

    io.BackendPlatformUserData = (void*) bd;
    io.BackendPlatformName = bd->platformName.data();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;    // We can create multi-viewports on the Platform side (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can call io.AddMouseViewportEvent() with correct data (optional)

    bd->context = ImGui::GetCurrentContext();

    // TODO: this should be the first window created in the constructor, to be initialized properly for ImGui
    std::shared_ptr<Window> window;

    bd->window = window;
    bd->deltaTime = 0.0;

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
    ImGui_ImplDirk_ViewportData* vd = IM_NEW(ImGui_ImplDirk_ViewportData)();
    vd->window = bd->window;
    vd->windowOwned = false;

    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    main_viewport->PlatformUserData = vd;
    main_viewport->PlatformHandle = (void*) bd->window.get();
    IM_UNUSED(main_viewport);
}

void Platform::ImGui_Shutdown() {
    ImGui_ImplDirk_Data* bd = ImGui_GetBackendData();
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

ImGui_ImplDirk_Data* Platform::ImGui_GetBackendData() {
    return ImGui::GetCurrentContext() ? (ImGui_ImplDirk_Data*) ImGui::GetIO().BackendPlatformUserData : nullptr;
}

ImGui_ImplDirk_Data* Platform::ImGui_GetBackendData(std::shared_ptr<Window> window) {
    ImGuiContext* ctx = contextMap.at(window);
    return (ImGui_ImplDirk_Data*) ImGui::GetIO(ctx).BackendPlatformUserData;
}

void Platform::ImGui_UpdateMonitors() {
    // TODO: setup detecting monitors
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

} // namespace dirk::Platform
