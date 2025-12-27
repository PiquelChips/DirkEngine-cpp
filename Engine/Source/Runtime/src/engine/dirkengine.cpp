#include "engine/dirkengine.hpp"

#include "common.hpp"
#include "engine/world.hpp"
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

        static constexpr std::string_view iniFilename = SAVED_PATH "/imgui.ini";
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
        renderer->initImGui(platform->createTempSurface(renderer->getResources().instance));
    }

    // initial engine state
    {
        auto viewport = renderer->createViewport(ViewportCreateInfo{
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

        tick(deltaTime);
    }

    renderer->shutdownImGui();
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

void DirkEngine::tick(float deltaTime) {
    platform->tick(deltaTime);
    // in case we fail platform event polling
    if (isRequestingExit())
        return;

    world->tick(deltaTime);

    renderer->render();
}

float DirkEngine::captureDeltaTime() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto duration = currentTime - lastTick;
    float deltaTime = std::chrono::duration<float>(duration).count();

    lastTick = currentTime;

    // TODO: have main editor engine ImGui window that displays these things
    // DIRK_LOG(LogDirkEngine, INFO) << "delta time: " << deltaTime;
    // DIRK_LOG(LogDirkEngine, INFO) << "fps: " << 1.0 / deltaTime;

    return deltaTime;
}

} // namespace dirk
