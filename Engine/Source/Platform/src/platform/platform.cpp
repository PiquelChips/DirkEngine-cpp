#include "platform/platform.hpp"
#include "common.hpp"
#include "platform/monitor.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_handles.hpp"

#ifdef PLATFORM_LINUX
#include "platform/linux/linux.hpp"
#endif

#include "imgui.h"
#include "input/keys.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace dirk::Platform {

DEFINE_LOG_CATEGORY(LogPlatform)
DEFINE_LOG_CATEGORY(LogImGui)

Platform::Platform(const PlatformCreateInfo& createInfo)
    : appName(createInfo.appName) {
#ifdef PLATFORM_LINUX
    platformImpl = std::make_unique<Linux::LinuxPlatformImpl>(createInfo, *this);
#else
#error "no platform specified"
#endif
}

void Platform::initImGui() {
    ImGuiPlatformData* bd = IM_NEW(ImGuiPlatformData)();

    ImGuiViewportPlatformData* vd = IM_NEW(ImGuiViewportPlatformData)();
    vd->window = platformImpl->createPlatformWindow(WindowCreateInfo{ .title = appName, .visible = true });
    vd->windowOwned = false;

    bd->mainWindow = vd->window.get();
    check(bd->mainWindow);
    bd->focusedWindow = bd->mainWindow;

    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    checkm(io.BackendPlatformUserData == nullptr, "Already initialized a platform backend!");

    io.BackendPlatformUserData = bd;
    io.BackendPlatformName = bd->platformName.data();

    // TODO: (ImGui) Enable all features
    // io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    // io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    // io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    // io.BackendFlags |= ImGuiBackendFlags_HasParentViewport;
    io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
    io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // TODO

    bd->context = ImGui::GetCurrentContext();
    bd->platform = this;

    contextMap[bd->mainWindow] = bd->context;

    // platform support
    ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
    platformIO.Platform_CreateWindow = ImGui_CreateWindow;
    platformIO.Platform_DestroyWindow = ImGui_DestroyWindow;
    platformIO.Platform_ShowWindow = ImGui_ShowWindow;
    platformIO.Platform_SetWindowPos = ImGui_SetWindowPos;
    platformIO.Platform_GetWindowPos = ImGui_GetWindowPos;
    platformIO.Platform_SetWindowSize = ImGui_SetWindowSize;
    platformIO.Platform_GetWindowSize = ImGui_GetWindowSize;
    platformIO.Platform_GetWindowFramebufferScale = ImGui_GetWindowFramebufferScale;
    platformIO.Platform_SetWindowFocus = ImGui_SetWindowFocus;
    platformIO.Platform_GetWindowFocus = ImGui_GetWindowFocus;
    platformIO.Platform_GetWindowMinimized = [](ImGuiViewport* vp) { return false; };
    platformIO.Platform_SetWindowTitle = ImGui_SetWindowTitle;
    platformIO.Platform_SetWindowAlpha = [](ImGuiViewport*, float) {};
    platformIO.Platform_CreateVkSurface = ImGui_CreateVkSurface;

    platformIO.Platform_GetClipboardTextFn = ImGui_GetClipboardText;
    platformIO.Platform_SetClipboardTextFn = ImGui_SetClipboardText;

    // TODO: setup cursors

    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    mainViewport->PlatformUserData = vd;
    mainViewport->PlatformHandle = bd->mainWindow->getPlatformHandle();
    IM_UNUSED(mainViewport);
}

void Platform::tick(float deltaTime) {
    ImGuiIO& io = ImGui::GetIO();
    ImGuiPlatformData* bd = getBackendData();
    check(bd);

    io.DeltaTime = deltaTime;
    bd->mouseIgnoreButtonUp = false;

    // TODO: framebuffer scale
    /*
    auto size = bd->mainWindow->getSize();
    io.DisplaySize = ImVec2(size.width, size.height);
    // Apple only
    auto fbSize = bd->window->getFramebufferSize();
    io.DisplayFramebufferScale = ImVec2((float) fbSize.width / (float) size.width, (float) fbSize.height / (float) size.height);
    */

    platformImpl->pollPlatformEvents();
}

void Platform::shutdownImGui() {
    ImGuiPlatformData* bd = getBackendData();
    checkm(bd != nullptr, "No platform backend to shutdown, or already shutdown?");

    ImGuiIO& io = ImGui::GetIO();
    ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();

    ImGui::DestroyPlatformWindows();

    io.BackendPlatformName = nullptr;
    io.BackendPlatformUserData = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos | ImGuiBackendFlags_HasGamepad | ImGuiBackendFlags_PlatformHasViewports | ImGuiBackendFlags_HasMouseHoveredViewport);
    platformIO.ClearPlatformHandlers();
    contextMap.erase(bd->mainWindow);
    IM_DELETE(bd);
}

Monitor& Platform::createMonitor(void* platformHandle) {
    monitors.push_back(std::make_unique<Monitor>(platformHandle, *this));
    return *monitors.back();
}

void Platform::ImGui_CreateWindow(ImGuiViewport* viewport) {
    DIRK_LOG(LogImGui, DEBUG, "creating window")
    ImGuiPlatformData* bd = getBackendData();
    ImGuiViewportPlatformData* vd = IM_NEW(ImGuiViewportPlatformData)();
    viewport->PlatformUserData = vd;

    // Workaround for Linux: ignore mouse up events corresponding to losing focus of the previously focused window (#7733, #3158, #7922)
    bd->mouseIgnoreButtonUpWaitForFocusLoss = true;

    WindowCreateInfo createInfo{
        .title = "No Title Yet",
        .size = { (uint32_t) viewport->Size.x, (uint32_t) viewport->Size.y },
        .focused = false,
        .visible = false,
        .decorated = (viewport->Flags & ImGuiViewportFlags_NoDecoration) ? false : true,
        .floating = (viewport->Flags & ImGuiViewportFlags_TopMost) ? true : false,
    };

    vd->window = bd->platform->platformImpl->createPlatformWindow(createInfo);
    vd->windowOwned = true;
    bd->platform->contextMap[vd->window.get()] = bd->context;
    viewport->PlatformHandle = (void*) vd->window->getPlatformHandle();

    vd->window->setPosition({ viewport->Pos.x, viewport->Pos.y });
}

void Platform::ImGui_DestroyWindow(ImGuiViewport* viewport) {
    ImGuiPlatformData* bd = getBackendData();
    if (ImGuiViewportPlatformData* vd = (ImGuiViewportPlatformData*) viewport->PlatformUserData) {
        if (vd->windowOwned) {
            // Release any keys that were pressed in the window being destroyed and are still held down,
            // because we will not receive any release events after window is destroyed.
            for (int i = 0; i < bd->keyOwnerWindows.size(); i++)
                if (bd->keyOwnerWindows[i] == vd->window.get())
                    bd->platform->keyCallback(*vd->window, (Input::Key) i, Input::KeyState::Released);

            bd->platform->contextMap.erase(vd->window.get());
        }
        vd->window = nullptr;
        IM_DELETE(vd);
    }
    viewport->PlatformUserData = viewport->PlatformHandle = nullptr;
}

void Platform::ImGui_ShowWindow(ImGuiViewport* viewport) {
    ImGuiViewportPlatformData* vd = static_cast<ImGuiViewportPlatformData*>(viewport->PlatformUserData);
    vd->window->show();
}

void Platform::ImGui_SetWindowPos(ImGuiViewport* viewport, ImVec2 pos) {
    ImGuiViewportPlatformData* vd = (ImGuiViewportPlatformData*) viewport->PlatformUserData;
    vd->ignoreWindowPosEventFrame = ImGui::GetFrameCount();
    vd->window->setPosition({ pos.x, pos.y });
}

ImVec2 Platform::ImGui_GetWindowPos(ImGuiViewport* viewport) {
    ImGuiViewportPlatformData* vd = (ImGuiViewportPlatformData*) viewport->PlatformUserData;
    auto pos = vd->window->getPosition();
    return ImVec2(pos.x, pos.y);
}

void Platform::ImGui_SetWindowSize(ImGuiViewport* viewport, ImVec2 size) {
    ImGuiViewportPlatformData* vd = (ImGuiViewportPlatformData*) viewport->PlatformUserData;
    vd->ignoreWindowSizeEventFrame = ImGui::GetFrameCount();
    vd->window->setSize({ (uint32_t) size.x, (uint32_t) size.y });
}

ImVec2 Platform::ImGui_GetWindowSize(ImGuiViewport* viewport) {
    ImGuiViewportPlatformData* vd = (ImGuiViewportPlatformData*) viewport->PlatformUserData;
    auto size = vd->window->getSize();
    return ImVec2(size.width, size.height);
}

ImVec2 Platform::ImGui_GetWindowFramebufferScale(ImGuiViewport* viewport) {
    // TODO: framebuffer scale stuff
    /**
    ImGuiViewportPlatformData* vd = (ImGuiViewportPlatformData*) viewport->PlatformUserData;
    ImVec2 framebuffer_scale;
    ImGui_ImplGlfw_GetWindowSizeAndFramebufferScale(vd->Window, nullptr, &framebuffer_scale);
    return framebuffer_scale;
    */
    return ImVec2(1.f, 1.f);
}

void Platform::ImGui_SetWindowFocus(ImGuiViewport* viewport) {
    ImGuiPlatformData* bd = getBackendData();
    ImGuiViewportPlatformData* vd = (ImGuiViewportPlatformData*) viewport->PlatformUserData;
    vd->window->focus();
}

bool Platform::ImGui_GetWindowFocus(ImGuiViewport* viewport) {
    ImGuiViewportPlatformData* vd = (ImGuiViewportPlatformData*) viewport->PlatformUserData;
    return vd->window->isFocused();
}

void Platform::ImGui_SetWindowTitle(ImGuiViewport* viewport, const char* title) {
    ImGuiViewportPlatformData* vd = (ImGuiViewportPlatformData*) viewport->PlatformUserData;
    vd->window->setTitle(title);
}

int Platform::ImGui_CreateVkSurface(ImGuiViewport* viewport, ImU64 instance, const void*, ImU64* outSurface) {
    ImGuiPlatformData* bd = getBackendData();
    ImGuiViewportPlatformData* vd = (ImGuiViewportPlatformData*) viewport->PlatformUserData;
    IM_UNUSED(bd);

    vd->window->createVulkanSurface((VkInstance) instance, (VkSurfaceKHR*) outSurface);
    return (int) vk::Result::eSuccess;
}

const char* Platform::ImGui_GetClipboardText(ImGuiContext* ctx) {
    ImGuiPlatformData* data = getBackendData();
    check(data->context = ctx);

    return data->platform->getClipboardText().data();
}

void Platform::ImGui_SetClipboardText(ImGuiContext* ctx, const char* text) {
    ImGuiPlatformData* data = getBackendData();
    check(data->context = ctx);
    data->platform->setClipboardText(text);
}

void Platform::windowSizeCallback(PlatformWindowImpl& window, vk::Extent2D inSize) {
    if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(window.getPlatformHandle())) {
        if (ImGuiViewportPlatformData* vd = (ImGuiViewportPlatformData*) viewport->PlatformUserData) {
            bool ignore_event = (ImGui::GetFrameCount() <= vd->ignoreWindowSizeEventFrame + 1);
            if (ignore_event)
                return;
        }
        viewport->PlatformRequestResize = true;
    }
}

void Platform::windowMoveCallback(PlatformWindowImpl& window) {
    if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(window.getPlatformHandle())) {
        if (ImGuiViewportPlatformData* vd = (ImGuiViewportPlatformData*) viewport->PlatformUserData) {
            bool ignore_event = (ImGui::GetFrameCount() <= vd->ignoreWindowPosEventFrame + 1);
            if (ignore_event)
                return;
        }
        viewport->PlatformRequestMove = true;
    }
}

void Platform::windowCloseCallback(PlatformWindowImpl& window) {
    auto* bd = getBackendData();
    if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(window.getPlatformHandle()))
        viewport->PlatformRequestClose = true;

    if (&window == bd->mainWindow)
        gEngine->exit("main window closed");
}

void Platform::focusWindowCallback(PlatformWindowImpl& window) {
    {
        ImGuiPlatformData* bd = getBackendData();

        // Workaround for Linux: when losing focus with MouseIgnoreButtonUpWaitForFocusLoss set, we will temporarily ignore subsequent Mouse Up events
        bd->mouseIgnoreButtonUp = bd->mouseIgnoreButtonUpWaitForFocusLoss;
        bd->mouseIgnoreButtonUpWaitForFocusLoss = false;

        ImGuiIO& io = ImGui::GetIO(bd->context);
        io.AddFocusEvent(false);
    }
    {
        ImGuiPlatformData* bd = getBackendData();

        // Workaround for Linux: when losing focus with MouseIgnoreButtonUpWaitForFocusLoss set, we will temporarily ignore subsequent Mouse Up events
        bd->mouseIgnoreButtonUp = false;
        bd->mouseIgnoreButtonUpWaitForFocusLoss = false;

        ImGuiIO& io = ImGui::GetIO(bd->context);
        io.AddFocusEvent(true);
    }
}

void Platform::cursorPosCallback(PlatformWindowImpl& window, glm::vec2 pos) {
    ImGuiPlatformData* bd = getBackendData();
    ImGuiIO& io = ImGui::GetIO(bd->context);

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        auto windowPos = window.getPosition();
        pos.x += windowPos.x;
        pos.y += windowPos.y;
    }
    io.AddMousePosEvent(pos.x, pos.y);
}

void Platform::mouseButtonCallback(PlatformWindowImpl& window, Input::MouseButton button, Input::KeyState action) {
    ImGuiPlatformData* bd = getBackendData();
    ImGuiIO& io = ImGui::GetIO(bd->context);

    // Workaround for Linux: ignore mouse up events which are following an focus loss following a viewport creation
    if (bd->mouseIgnoreButtonUp && action == Input::KeyState::Released)
        return;

    io.AddMouseButtonEvent((int) button, action == Input::KeyState::Pressed);
}

void Platform::mouseScrollCallback(PlatformWindowImpl& window, glm::vec2 offset) {
    ImGuiPlatformData* bd = getBackendData();
    ImGuiIO& io = ImGui::GetIO(bd->context);

    io.AddMouseWheelEvent(offset.x, offset.y);
}

void Platform::keyCallback(PlatformWindowImpl& window, Input::Key key, Input::KeyState action) {
    ImGuiPlatformData* bd = getBackendData();
    ImGuiIO& io = ImGui::GetIO(bd->context);

    if (action != Input::KeyState::Pressed && action != Input::KeyState::Released)
        return;

    bd->keyOwnerWindows[key] = (action == Input::KeyState::Pressed) ? &window : nullptr;

    io.AddKeyEvent(keyToImGuiKey(key), (action == Input::KeyState::Pressed));
}

void Platform::charCallback(PlatformWindowImpl& window, unsigned int c) {
    ImGuiPlatformData* bd = getBackendData();
    ImGuiIO& io = ImGui::GetIO(bd->context);

    io.AddInputCharacter(c);
}

ImGuiPlatformData* Platform::getBackendData() {
    return ImGui::GetCurrentContext() ? (ImGuiPlatformData*) ImGui::GetIO().BackendPlatformUserData : nullptr;
}

void Platform::updateMonitors() {
    ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();

    auto& monitors = getMonitors();
    if (monitors.size() == 0) // Preserve existing monitor list if there are none. Happens on macOS sleeping
        return;

    platformIO.Monitors.resize(0);
    for (auto& monitor : monitors) {
        ImGuiPlatformMonitor imGuiMonitor;

        auto position = monitor->getPosition();
        auto& vidMode = monitor->getVideoMode();
        imGuiMonitor.MainPos = imGuiMonitor.WorkPos = ImVec2(position.x, position.y);
        imGuiMonitor.MainSize = imGuiMonitor.WorkSize = ImVec2(vidMode.size.width, vidMode.size.height);

        imGuiMonitor.DpiScale = 1.f;
        imGuiMonitor.PlatformHandle = monitor->getPlatformHandle();
        platformIO.Monitors.push_back(imGuiMonitor);
    }
}

// clang-format off
ImGuiKey Platform::keyToImGuiKey(Input::Key key)
{
    switch (key)
    {
    case Input::Key::Tab: return ImGuiKey_Tab;
    case Input::Key::Left: return ImGuiKey_LeftArrow;
    case Input::Key::Right: return ImGuiKey_RightArrow;
    case Input::Key::Up: return ImGuiKey_UpArrow;
    case Input::Key::Down: return ImGuiKey_DownArrow;
    case Input::Key::PageUp: return ImGuiKey_PageUp;
    case Input::Key::PageDown: return ImGuiKey_PageDown;
    case Input::Key::Home: return ImGuiKey_Home;
    case Input::Key::End: return ImGuiKey_End;
    case Input::Key::Insert: return ImGuiKey_Insert;
    case Input::Key::Delete: return ImGuiKey_Delete;
    case Input::Key::Backspace: return ImGuiKey_Backspace;
    case Input::Key::Space: return ImGuiKey_Space;
    case Input::Key::Enter: return ImGuiKey_Enter;
    case Input::Key::Escape: return ImGuiKey_Escape;
    case Input::Key::Apostrophe: return ImGuiKey_Apostrophe;
    case Input::Key::Comma: return ImGuiKey_Comma;
    case Input::Key::Minus: return ImGuiKey_Minus;
    case Input::Key::Period: return ImGuiKey_Period;
    case Input::Key::Slash: return ImGuiKey_Slash;
    case Input::Key::Semicolon: return ImGuiKey_Semicolon;
    case Input::Key::Equal: return ImGuiKey_Equal;
    case Input::Key::LeftBracket: return ImGuiKey_LeftBracket;
    case Input::Key::Backslash: return ImGuiKey_Backslash;
    case Input::Key::World1: return ImGuiKey_Oem102;
    case Input::Key::World2: return ImGuiKey_Oem102;
    case Input::Key::RightBracket: return ImGuiKey_RightBracket;
    case Input::Key::GraveAccent: return ImGuiKey_GraveAccent;
    case Input::Key::CapsLock: return ImGuiKey_CapsLock;
    case Input::Key::ScrollLock: return ImGuiKey_ScrollLock;
    case Input::Key::NumLock: return ImGuiKey_NumLock;
    case Input::Key::PrintScreen: return ImGuiKey_PrintScreen;
    case Input::Key::Pause: return ImGuiKey_Pause;
    case Input::Key::KP0: return ImGuiKey_Keypad0;
    case Input::Key::KP1: return ImGuiKey_Keypad1;
    case Input::Key::KP2: return ImGuiKey_Keypad2;
    case Input::Key::KP3: return ImGuiKey_Keypad3;
    case Input::Key::KP4: return ImGuiKey_Keypad4;
    case Input::Key::KP5: return ImGuiKey_Keypad5;
    case Input::Key::KP6: return ImGuiKey_Keypad6;
    case Input::Key::KP7: return ImGuiKey_Keypad7;
    case Input::Key::KP8: return ImGuiKey_Keypad8;
    case Input::Key::KP9: return ImGuiKey_Keypad9;
    case Input::Key::KPDecimal: return ImGuiKey_KeypadDecimal;
    case Input::Key::KPDivide: return ImGuiKey_KeypadDivide;
    case Input::Key::KPMultiply: return ImGuiKey_KeypadMultiply;
    case Input::Key::KPSubtract: return ImGuiKey_KeypadSubtract;
    case Input::Key::KPAdd: return ImGuiKey_KeypadAdd;
    case Input::Key::KPEnter: return ImGuiKey_KeypadEnter;
    case Input::Key::KPEqual: return ImGuiKey_KeypadEqual;
    case Input::Key::LeftShift: return ImGuiKey_LeftShift;
    case Input::Key::LeftControl: return ImGuiKey_LeftCtrl;
    case Input::Key::LeftAlt: return ImGuiKey_LeftAlt;
    case Input::Key::LeftSuper: return ImGuiKey_LeftSuper;
    case Input::Key::RightShift: return ImGuiKey_RightShift;
    case Input::Key::RightControl: return ImGuiKey_RightCtrl;
    case Input::Key::RightAlt: return ImGuiKey_RightAlt;
    case Input::Key::RightSuper: return ImGuiKey_RightSuper;
    case Input::Key::Menu: return ImGuiKey_Menu;
    case Input::Key::D0: return ImGuiKey_0;
    case Input::Key::D1: return ImGuiKey_1;
    case Input::Key::D2: return ImGuiKey_2;
    case Input::Key::D3: return ImGuiKey_3;
    case Input::Key::D4: return ImGuiKey_4;
    case Input::Key::D5: return ImGuiKey_5;
    case Input::Key::D6: return ImGuiKey_6;
    case Input::Key::D7: return ImGuiKey_7;
    case Input::Key::D8: return ImGuiKey_8;
    case Input::Key::D9: return ImGuiKey_9;
    case Input::Key::A: return ImGuiKey_A;
    case Input::Key::B: return ImGuiKey_B;
    case Input::Key::C: return ImGuiKey_C;
    case Input::Key::D: return ImGuiKey_D;
    case Input::Key::E: return ImGuiKey_E;
    case Input::Key::F: return ImGuiKey_F;
    case Input::Key::G: return ImGuiKey_G;
    case Input::Key::H: return ImGuiKey_H;
    case Input::Key::I: return ImGuiKey_I;
    case Input::Key::J: return ImGuiKey_J;
    case Input::Key::K: return ImGuiKey_K;
    case Input::Key::L: return ImGuiKey_L;
    case Input::Key::M: return ImGuiKey_M;
    case Input::Key::N: return ImGuiKey_N;
    case Input::Key::O: return ImGuiKey_O;
    case Input::Key::P: return ImGuiKey_P;
    case Input::Key::Q: return ImGuiKey_Q;
    case Input::Key::R: return ImGuiKey_R;
    case Input::Key::S: return ImGuiKey_S;
    case Input::Key::T: return ImGuiKey_T;
    case Input::Key::U: return ImGuiKey_U;
    case Input::Key::V: return ImGuiKey_V;
    case Input::Key::W: return ImGuiKey_W;
    case Input::Key::X: return ImGuiKey_X;
    case Input::Key::Y: return ImGuiKey_Y;
    case Input::Key::Z: return ImGuiKey_Z;
    case Input::Key::F1: return ImGuiKey_F1;
    case Input::Key::F2: return ImGuiKey_F2;
    case Input::Key::F3: return ImGuiKey_F3;
    case Input::Key::F4: return ImGuiKey_F4;
    case Input::Key::F5: return ImGuiKey_F5;
    case Input::Key::F6: return ImGuiKey_F6;
    case Input::Key::F7: return ImGuiKey_F7;
    case Input::Key::F8: return ImGuiKey_F8;
    case Input::Key::F9: return ImGuiKey_F9;
    case Input::Key::F10: return ImGuiKey_F10;
    case Input::Key::F11: return ImGuiKey_F11;
    case Input::Key::F12: return ImGuiKey_F12;
    case Input::Key::F13: return ImGuiKey_F13;
    case Input::Key::F14: return ImGuiKey_F14;
    case Input::Key::F15: return ImGuiKey_F15;
    case Input::Key::F16: return ImGuiKey_F16;
    case Input::Key::F17: return ImGuiKey_F17;
    case Input::Key::F18: return ImGuiKey_F18;
    case Input::Key::F19: return ImGuiKey_F19;
    case Input::Key::F20: return ImGuiKey_F20;
    case Input::Key::F21: return ImGuiKey_F21;
    case Input::Key::F22: return ImGuiKey_F22;
    case Input::Key::F23: return ImGuiKey_F23;
    case Input::Key::F24: return ImGuiKey_F24;
    default:
        DIRK_LOG(LogPlatform, ERROR, "no imgui equivalent for key {}", (uint16_t) key)
        return ImGuiKey_None;
    }
}
// clang-format on

} // namespace dirk::Platform
