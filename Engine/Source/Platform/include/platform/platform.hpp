#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

#include "backends/imgui_impl_vulkan.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "input/keys.hpp"
#include "window.hpp"

namespace dirk::Platform {

struct Cursor {};
struct PlatformCreateInfo {};

struct ImGui_ImplDirk_Data {
    ImGuiContext* context;
    static constexpr std::string_view platformName = "imgui_impl_dirk";
    std::shared_ptr<Window> window;
    double deltaTime;
    std::array<Cursor, ImGuiMouseCursor_COUNT> mouseCursors;

    glm::vec2 lastValidMousePos;
    std::array<std::shared_ptr<Window>, Input::KeyLast> keyOwnerWindows; // keys used as indexes, window is which window currently has that key
    std::shared_ptr<Window> mouseWindow;                                 // the window the mouse is currenly on (if nullptr, mouse not on any window)

    bool mouseIgnoreButtonUpWaitForFocusLoss;
    bool mouseIgnoreButtonUp;

    ImGui_ImplDirk_Data() { memset((void*) this, 0, sizeof(*this)); }
};

struct ImGui_ImplDirk_ViewportData {
    std::shared_ptr<Window> window;
    bool windowOwned;
};

class Platform {
public:
    Platform(const PlatformCreateInfo& createInfo);

    void ImGui_Init();
    void ImGui_Shutdown();

private:
    // platform funcs used by ImGui
    static void CreateWindow(ImGuiViewport* vp);                    // . . U . .  // Create a new platform window for the given viewport
    static void DestroyWindow(ImGuiViewport* vp);                   // N . U . D  //
    static void ShowWindow(ImGuiViewport* vp);                      // . . U . .  // Newly created windows are initially hidden so SetWindowPos/Size/Title can be called on them before showing the window
    static void SetWindowPos(ImGuiViewport* vp, ImVec2 pos);        // . . U . .  // Set platform window position (given the upper-left corner of client area)
    static ImVec2 GetWindowPos(ImGuiViewport* vp);                  // N . . . .  //
    static void SetWindowSize(ImGuiViewport* vp, ImVec2 size);      // . . U . .  // Set platform window client area size (ignoring OS decorations such as OS title bar etc.)
    static ImVec2 GetWindowSize(ImGuiViewport* vp);                 // N . . . .  // Get platform window client area size
    static ImVec2 GetWindowFramebufferScale(ImGuiViewport* vp);     // N . . . .  // Return viewport density. Always 1,1 on Windows, often 2,2 on Retina display on macOS/iOS. MUST BE INTEGER VALUES.
    static void SetWindowFocus(ImGuiViewport* vp);                  // N . . . .  // Move window to front and set input focus
    static bool GetWindowFocus(ImGuiViewport* vp);                  // . . U . .  //
    static bool GetWindowMinimized(ImGuiViewport* vp);              // N . . . .  // Get platform window minimized state. When minimized, we generally won't attempt to get/set size and contents will be culled more easily
    static void SetWindowTitle(ImGuiViewport* vp, const char* str); // . . U . .  // Set platform window title (given an UTF-8 string)
    static int CreateVkSurface(ImGuiViewport* vp, ImU64 vk_inst, const void* vk_allocators, ImU64* out_vk_surface);

    // callbacks for platform events
    void focusWindowCallback(std::shared_ptr<Window> window, bool focused);
    void cursorEnterCallback(std::shared_ptr<Window> window, bool entered);
    void cursorPosCallback(std::shared_ptr<Window> window, glm::vec2 pos);
    void mouseButtonCallback(std::shared_ptr<Window> window, int button, int action, int mods);
    void mouseScrollCallback(std::shared_ptr<Window> window, glm::vec2 offset);
    void keyCallback(std::shared_ptr<Window> window, int keycode, int scancode, int action, int mods);
    void charCallback(std::shared_ptr<Window> window, unsigned int c);

private:
    ImGui_ImplDirk_Data* ImGui_GetBackendData();
    ImGui_ImplDirk_Data* ImGui_GetBackendData(std::shared_ptr<Window> window);

    void ImGui_UpdateMonitors();

    std::unordered_map<std::shared_ptr<Window>, ImGuiContext*> contextMap;

public:
    static std::vector<const char*> getRequiredExtensions();
};

} // namespace dirk::Platform
