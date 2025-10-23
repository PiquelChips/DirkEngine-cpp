#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

#include "imgui.h"
#include "imgui_internal.h"
#include "input/keys.hpp"
#include "window.hpp"

namespace dirk::Platform {

struct Cursor {};
struct PlatformCreateInfo {};

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

class Platform {
public:
    Platform(const PlatformCreateInfo& createInfo);
    ~Platform();

    void tick(float deltaTime);

private:
    // platform funcs used by ImGui
    static void ImGui_CreateWindow(ImGuiViewport* viewport);                      // . . U . .  // Create a new platform window for the given viewport
    static void ImGui_DestroyWindow(ImGuiViewport* viewport);                     // N . U . D  //
    static void ImGui_ShowWindow(ImGuiViewport* viewport);                        // . . U . .  // Newly created windows are initially hidden so SetWindowPos/Size/Title can be called on them before showing the window
    static void ImGui_SetWindowPos(ImGuiViewport* viewport, ImVec2 pos);          // . . U . .  // Set platform window position (given the upper-left corner of client area)
    static ImVec2 ImGui_GetWindowPos(ImGuiViewport* viewport);                    // N . . . .  //
    static void ImGui_SetWindowSize(ImGuiViewport* viewport, ImVec2 size);        // . . U . .  // Set platform window client area size (ignoring OS decorations such as OS title bar etc.)
    static ImVec2 ImGui_GetWindowSize(ImGuiViewport* viewport);                   // N . . . .  // Get platform window client area size
    static ImVec2 ImGui_GetWindowFramebufferScale(ImGuiViewport* viewport);       // N . . . .  // Return viewport density. Always 1,1 on Windows, often 2,2 on Retina display on macOS/iOS. MUST BE INTEGER VALUES.
    static void ImGui_SetWindowFocus(ImGuiViewport* viewport);                    // N . . . .  // Move window to front and set input focus
    static bool ImGui_GetWindowFocus(ImGuiViewport* viewport);                    // . . U . .  //
    static bool ImGui_GetWindowMinimized(ImGuiViewport* viewport);                // N . . . .  // Get platform window minimized state. When minimized, we generally won't attempt to get/set size and contents will be culled more easily
    static void ImGui_SetWindowTitle(ImGuiViewport* viewport, const char* title); // . . U . .  // Set platform window title (given an UTF-8 string)
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

public:
    static std::vector<const char*> getRequiredExtensions();
    static ImGuiKey keyToImGuiKey(Input::Key key);
};

} // namespace dirk::Platform
