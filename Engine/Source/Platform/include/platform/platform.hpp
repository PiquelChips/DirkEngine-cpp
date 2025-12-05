#pragma once

#include "common.hpp"
#include "input/keys.hpp"
#include "monitor.hpp"

#include "imgui.h"
#include "imgui_internal.h"
#include "vulkan/vulkan_handles.hpp"

#include <array>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace dirk::Platform {

DECLARE_LOG_CATEGORY_EXTERN(LogPlatform)
DECLARE_LOG_CATEGORY_EXTERN(LogImGui)

struct WindowCreateInfo {
    std::string_view title;
    vk::Extent2D size = { 550, 680 };

    bool focused;
    bool visible;
    bool decorated;
    bool floating;
};

class PlatformWindowImpl {
public:
    virtual ~PlatformWindowImpl() = default;

    virtual void show() = 0;
    virtual void hide() = 0;

    virtual vk::SurfaceKHR getVulkanSurface(vk::Instance instance) = 0;
    virtual void createVulkanSurface(VkInstance instance, VkSurfaceKHR* surface) = 0;
    virtual void* getPlatformHandle() = 0;

    virtual vk::Extent2D getSize() = 0;
    virtual void setSize(vk::Extent2D inSize) = 0;

    virtual glm::vec2 getPosition() = 0;
    virtual void setPosition(const glm::vec2 inPosition) = 0;

    virtual std::string_view getTitle() = 0;
    virtual void setTitle(std::string_view inTitle) = 0;

    virtual bool isFocused() = 0;
    virtual void focus() = 0;

    virtual bool isDecorated() = 0;
    virtual void setDecorated(bool inDecorated) = 0;
};

struct PlatformCreateInfo {
    std::string_view appName;
};

struct ImGuiViewportPlatformData {
    std::unique_ptr<PlatformWindowImpl> window;

    bool windowOwned;
    int ignoreWindowSizeEventFrame;
    int ignoreWindowPosEventFrame;

    ImGuiViewportPlatformData() {
        memset((void*) this, 0, sizeof(*this));
        ignoreWindowSizeEventFrame = ignoreWindowPosEventFrame = -1;
    }
    ~ImGuiViewportPlatformData() { IM_ASSERT(window == nullptr); }
};

struct ImGuiPlatformData {
    ImGuiContext* context;
    Platform* platform;

    PlatformWindowImpl* mainWindow;
    PlatformWindowImpl* focusedWindow;

    static constexpr std::string_view platformName = "imgui_impl_dirk";

    std::array<PlatformWindowImpl*, Input::KeyLast> keyOwnerWindows; // keys used as indexes, window is which window currently has that key

    bool mouseIgnoreButtonUpWaitForFocusLoss;
    bool mouseIgnoreButtonUp;

    ImGuiPlatformData() { memset((void*) this, 0, sizeof(*this)); }
};

class PlatformImpl {
public:
    virtual ~PlatformImpl() = default;
    virtual void pollPlatformEvents() = 0;
    virtual std::unique_ptr<PlatformWindowImpl> createPlatformWindow(const WindowCreateInfo& createInfo) = 0;
    // TODO: remove
    virtual vk::SurfaceKHR createTempSurface(vk::Instance instance) = 0;

    virtual std::string_view getClipboardText() = 0;
    virtual void setClipboardText(const std::string& text) = 0;
};

class Platform {
public:
    Platform(const PlatformCreateInfo& createInfo);
    ~Platform();

    void initImGui();
    void tick(float deltaTime);
    void shutdownImGui();

    Monitor& createMonitor(void* platformHandle);
    vk::SurfaceKHR createTempSurface(vk::Instance instance) { return platformImpl->createTempSurface(instance); }
    // updated the ImGui monitors list with current platform monitors list
    void updateMonitors();

    std::vector<std::unique_ptr<Monitor>>& getMonitors() { return monitors; }

    std::string_view getClipboardText() { return platformImpl->getClipboardText(); }
    void setClipboardText(const std::string& text) { platformImpl->setClipboardText(text); }

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

    static const char* ImGui_GetClipboardText(ImGuiContext* ctx);
    static void ImGui_SetClipboardText(ImGuiContext* ctx, const char* text);

public:
    // callbacks for platform events
    void windowSizeCallback(PlatformWindowImpl& window, vk::Extent2D inSize);
    void windowMoveCallback(PlatformWindowImpl& window);
    void windowCloseCallback(PlatformWindowImpl& window);
    void focusWindowCallback(PlatformWindowImpl& window);
    void cursorPosCallback(PlatformWindowImpl& window, glm::vec2 pos);
    void mouseButtonCallback(PlatformWindowImpl& window, Input::MouseButton button, Input::KeyState action);
    void mouseScrollCallback(PlatformWindowImpl& window, glm::vec2 offset);
    void keyCallback(PlatformWindowImpl& window, Input::Key key, Input::KeyState action);
    void charCallback(PlatformWindowImpl& window, unsigned int c);

    static ImGuiPlatformData* getBackendData();

private:
    std::unordered_map<PlatformWindowImpl*, ImGuiContext*> contextMap;
    std::vector<std::unique_ptr<Monitor>> monitors;

    std::string_view appName;
    std::unique_ptr<PlatformImpl> platformImpl;

public:
    static ImGuiKey keyToImGuiKey(Input::Key key);
};

} // namespace dirk::Platform
