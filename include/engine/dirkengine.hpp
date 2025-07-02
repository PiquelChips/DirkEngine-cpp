#pragma once

#include <GLFW/glfw3.h>
#include <memory>
#include <string>

#include "logger.hpp"
#include "render/render.hpp"

class DirkEngine {

public:
    DirkEngine();

    int main();
    void exit(const std::string& reason);

    bool isRequestingExit() const noexcept { return requestingExit; }
    Logger* getLogger() const noexcept { return logger.get(); }

    RendererConfig RENDER_CONFIG{ 800, 600, "DirkEngine" };

private:
    int init();
    void tick();

    void cleanup();

private:
    std::unique_ptr<Logger> logger = nullptr;
    std::unique_ptr<Renderer> renderer = nullptr;

    bool requestingExit = false;
};
