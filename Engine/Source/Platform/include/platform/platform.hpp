#pragma once

#include "common.hpp"
#include "input/keys.hpp"
#include "monitor.hpp"
#include "window.hpp"

#include "imgui.h"
#include "imgui_internal.h"
#include "vulkan/vulkan_handles.hpp"

#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

namespace dirk::Platform {

DECLARE_LOG_CATEGORY_EXTERN(LogPlatform)
DECLARE_LOG_CATEGORY_EXTERN(LogImGui)

struct Cursor {};

struct PlatformCreateInfo {
    std::string_view appName;
};

struct ImGuiData {
    ImGuiContext* context;
    Platform* platform;
    static constexpr std::string_view platformName = "imgui_impl_dirk";
    Window* window;
    std::array<Cursor, ImGuiMouseCursor_COUNT> mouseCursors;

    std::array<Window*, Input::KeyLast> keyOwnerWindows; // keys used as indexes, window is which window currently has that key
    Window* mouseWindow;                                 // the window the mouse is currenly on (if nullptr, mouse not on any window)

    bool mouseIgnoreButtonUpWaitForFocusLoss;
    bool mouseIgnoreButtonUp;

    ImGuiData() { memset((void*) this, 0, sizeof(*this)); }
};

struct ImGuiViewportData {
    Window* window;
    bool windowOwned;
    int ignoreWindowSizeEventFrame;
    int ignoreWindowPosEventFrame;

    ImGuiViewportData() {
        memset((void*) this, 0, sizeof(*this));
        ignoreWindowSizeEventFrame = ignoreWindowPosEventFrame = -1;
    }
    ~ImGuiViewportData() { IM_ASSERT(window == nullptr); }
};

class PlatformImpl {
public:
    virtual ~PlatformImpl() = default;
    virtual void pollPlatformEvents() = 0;
    virtual std::unique_ptr<PlatformWindowImpl> createPlatformWindow(const WindowCreateInfo& createInfo) = 0;
    // TODO: remove
    virtual vk::SurfaceKHR createTempSurface(vk::Instance instance) = 0;
};

class Platform : public IPlatform {
public:
    Platform(const PlatformCreateInfo& createInfo);
    ~Platform();

    void initImGui();
    void tick(float deltaTime);
    void shutdownImGui();

    // clang-format off
    Window& getMainWindow() { check(windows[0]); return *windows[0]; }
    Window& getFocusedWindow() { check(focusedWindow); return *focusedWindow; }
    // clang-format on

    Monitor& createMonitor(void* platformHandle);
    vk::SurfaceKHR createTempSurface(vk::Instance instance) { return platformImpl->createTempSurface(instance); }
    // updated the ImGui monitors list with current platform monitors list
    void updateMonitors();

    std::vector<std::unique_ptr<Window>>& getWindows() { return windows; }
    std::vector<std::unique_ptr<Monitor>>& getMonitors() { return monitors; }

private:
    // platform funcs used by ImGui
    static void ImGui_CreateWindow(ImGuiViewport* viewport);
    static void ImGui_DestroyWindow(ImGuiViewport* viewport);
    static void ImGui_ShowWindow(ImGuiViewport* viewport);
    static void ImGui_SetWindowPos(ImGuiViewport* viewport, ImVec2 pos);
    static ImVec2 ImGui_GetWindowPos(ImGuiViewport* viewport);
    static void ImGui_SetWindowSize(ImGuiViewport* viewport, ImVec2 size);
    static ImVec2 ImGui_GetWindowSize(ImGuiViewport* viewport);
    static ImVec2 ImGui_GetWindowFramebufferScale(ImGuiViewport* viewport);
    static void ImGui_SetWindowFocus(ImGuiViewport* viewport);
    static bool ImGui_GetWindowFocus(ImGuiViewport* viewport);
    static void ImGui_SetWindowTitle(ImGuiViewport* viewport, const char* title);
    static int ImGui_CreateVkSurface(ImGuiViewport* viewport, ImU64 instance, const void*, ImU64* outSurface);

public:
    // callbacks for platform events
    void windowSizeCallback(Window& window, vk::Extent2D inSize);
    void windowMoveCallback(Window& window);
    void windowCloseCallback(Window& window);
    void focusWindowCallback(Window& window);
    void cursorPosCallback(Window& window, glm::vec2 pos);
    void mouseButtonCallback(Window& window, Input::MouseButton button, Input::KeyState action);
    void mouseScrollCallback(Window& window, glm::vec2 offset);
    void keyCallback(Window& window, Input::Key key, Input::KeyState action);
    void charCallback(Window& window, unsigned int c);

private:
    static ImGuiData* getBackendData();
    ImGuiData* getBackendData(Window& window);

    void updateMouseData();
    void updateMouseCursor();

    Window* createWindow(const WindowCreateInfo& createInfo);
    void destroyWindow(Window* window);

    std::unordered_map<Window*, ImGuiContext*> contextMap;
    std::vector<std::unique_ptr<Window>> windows;
    std::vector<std::unique_ptr<Monitor>> monitors;
    // the last window that was focused
    Window* focusedWindow;

    std::string_view appName;

    std::unique_ptr<PlatformImpl> platformImpl;

public:
    static ImGuiKey keyToImGuiKey(Input::Key key);
};

} // namespace dirk::Platform
