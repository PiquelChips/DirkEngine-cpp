#pragma once

#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

#include "common.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "input/keys.hpp"
#include "window.hpp"

namespace dirk::Platform {

struct Cursor {};

struct PlatformCreateInfo {
    std::string_view appName;
};

struct ImGuiData {
    ImGuiContext* context;
    Platform* platform;
    static constexpr std::string_view platformName = "imgui_impl_dirk";
    std::shared_ptr<Window> window;
    std::array<Cursor, ImGuiMouseCursor_COUNT> mouseCursors;

    glm::vec2 lastValidMousePos;
    std::array<std::shared_ptr<Window>, Input::KeyLast> keyOwnerWindows; // keys used as indexes, window is which window currently has that key
    std::shared_ptr<Window> mouseWindow;                                 // the window the mouse is currenly on (if nullptr, mouse not on any window)

    bool mouseIgnoreButtonUpWaitForFocusLoss;
    bool mouseIgnoreButtonUp;

    ImGuiData() { memset((void*) this, 0, sizeof(*this)); }
};

struct ImGuiViewportData {
    std::shared_ptr<Window> window;
    bool windowOwned;
    int ignoreWindowSizeEventFrame;
    int ignoreWindowPosEventFrame;

    ImGuiViewportData() {
        memset((void*) this, 0, sizeof(*this));
        ignoreWindowSizeEventFrame = ignoreWindowPosEventFrame = -1;
    }
    ~ImGuiViewportData() { IM_ASSERT(window == nullptr); }
};

class Platform : public IPlatform {
public:
    Platform(const PlatformCreateInfo& createInfo);
    ~Platform();

    void initImGui();
    void tick(float deltaTime);
    void shutdownImGui();

    std::shared_ptr<Window>& getMainWindow() { return windows[0]; }

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
    static bool ImGui_GetWindowMinimized(ImGuiViewport* viewport);
    static void ImGui_SetWindowTitle(ImGuiViewport* viewport, const char* title);
    static int ImGui_CreateVkSurface(ImGuiViewport* viewport, ImU64 instance, const void*, ImU64* outSurface);

    // callbacks for platform events
    void windowSizeCallback(std::shared_ptr<Window> window, vk::Extent2D inSize);
    void windowPosCallback(std::shared_ptr<Window> window, glm::vec2 inPos);
    void windowCloseCallback(std::shared_ptr<Window> window);
    void focusWindowCallback(std::shared_ptr<Window> window, bool focused);
    void cursorEnterCallback(std::shared_ptr<Window> window, bool entered);
    void cursorPosCallback(std::shared_ptr<Window> window, glm::vec2 pos);
    void mouseButtonCallback(std::shared_ptr<Window> window, Input::MouseButton button, Input::KeyState action);
    void mouseScrollCallback(std::shared_ptr<Window> window, glm::vec2 offset);
    void keyCallback(std::shared_ptr<Window> window, Input::Key key, Input::KeyState action);
    void charCallback(std::shared_ptr<Window> window, unsigned int c);

private:
    static ImGuiData* getBackendData();
    static ImGuiData* getBackendData(std::shared_ptr<Window> window);

    void updateMonitors();
    void updateMouseData();
    void updateMouseCursor();

    std::shared_ptr<Window> createWindow(const WindowCreateInfo& createInfo);
    void destroyWindow(std::shared_ptr<Window> window);
    void focusWindow(std::shared_ptr<Window> window);

    std::unordered_map<std::shared_ptr<Window>, ImGuiContext*> contextMap;
    std::vector<std::shared_ptr<Window>> windows;

    std::string_view appName;

public:
    static ImGuiKey keyToImGuiKey(Input::Key key);
};

std::vector<const char*> getRequiredExtensions();

} // namespace dirk::Platform
