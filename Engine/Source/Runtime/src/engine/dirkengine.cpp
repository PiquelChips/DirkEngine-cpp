#include "engine/dirkengine.hpp"

#include "common.hpp"
#include "engine/world.hpp"
#include "imgui.h"
#include "render/renderer.hpp"
#include "render/viewport.hpp"
#include "vulkan/vulkan_structs.hpp"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace dirk {

DEFINE_LOG_CATEGORY(LogEngine)

IEngine* gEngine;

DirkEngine::DirkEngine(const DirkEngineCreateInfo& createInfo) {
    gEngine = this;
    Logging::init();

    // main engine objects
    {
        renderer = std::make_unique<Renderer>();
        platform = std::make_unique<Platform::Platform>(createInfo.platformCreateInfo);
        renderer->init(platform->createTempSurface(renderer->getResources().instance));
        world = std::make_shared<World>(createInfo.actorCreateInfos);
    }

    // ImGui
    {
        DIRK_LOG(LogVulkan, DEBUG, "initlializing imgui");
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        io.ConfigDpiScaleFonts = true;
        io.ConfigDpiScaleViewports = true;

        static constexpr std::string_view iniFilename = SAVED_DIR "/imgui.ini";
        io.IniFilename = iniFilename.data();

        ImGui::StyleColorsDark();

        // Setup scaling
        ImGuiStyle& style = ImGui::GetStyle();

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        style.ScaleAllSizes(1.f);
        style.FontScaleDpi = 1.f;
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;

        platform->initImGui();
        renderer->ImGui_init(platform->createTempSurface(renderer->getResources().instance));
    }

    // initial engine state
    {
        auto& viewport = renderer->createViewport(ViewportCreateInfo{
            .name = "Hello World!",
            .size = vk::Extent2D(500, 500),
            .world = world,
        });
    }

    lastTick = std::chrono::high_resolution_clock::now();

    while (true) {
        float deltaTime = captureDeltaTime();

        if (isRequestingExit())
            break;

        if (!tick(deltaTime))
            break;
    }

    renderer->ImGui_shutdown();
    platform->shutdownImGui();
}

DirkEngine::~DirkEngine() {
    DIRK_LOG(LogEngine, INFO, "exiting");
    Logging::shutdown();
}

void DirkEngine::exit() {
    requestingExit = true;
}

void DirkEngine::exit(const std::string& reason) {
    DIRK_LOG(LogEngine, INFO, "engine exit has been requested with reason: {}", reason);
    this->exit();
}

bool DirkEngine::tick(float deltaTime) {
    platform->tick(deltaTime);
    // in case we fail platform event polling
    if (isRequestingExit())
        return false;

    world->tick(deltaTime);

    renderer->render();

    // ImGui
    {
        ImGuiIO& io = ImGui::GetIO();
        io.DeltaTime = deltaTime;
        renderer->ImGui_beginFrame();
        ImGui::NewFrame();
        renderImGui(deltaTime);
        ImGui::Render();
        renderer->ImGui_render();
    }

    return !isRequestingExit();
}

float DirkEngine::captureDeltaTime() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto duration = currentTime - lastTick;
    float deltaTime = std::chrono::duration<float>(duration).count();

    lastTick = currentTime;

    return deltaTime;
}

void DirkEngine::renderImGui(float deltaTime) {
    // Make main parent window
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4{ .0f, .0f, .0f, .0f });
    ImGui::Begin("DockSpace", nullptr, windowFlags);
    ImGui::PopStyleColor(); // MenuBarBg
    ImGui::PopStyleVar(3);

    ImGui::DockSpace(ImGui::GetID("DirkDockspace"));

    ImGui::Begin("Editor Panel");
    {
        ImGui::Text("FPS: %f", 1.0 / deltaTime);
        ImGui::Text("Frame Time: %f", deltaTime);

        ImGui::Checkbox("Show Demo Window", &showDemoWindow);
        ImGui::Checkbox("Show Style Editor", &showStyleEditor);
    }
    ImGui::End();

    if (showDemoWindow)
        ImGui::ShowDemoWindow();

    if (showStyleEditor) {
        ImGui::Begin("ImGui style editor", &showStyleEditor);
        ImGui::ShowStyleEditor();
        ImGui::End();
    }

    for (auto& viewport : renderer->getViewports()) {
        viewport->renderImGui();
    }

    ImGui::End();
}

} // namespace dirk
