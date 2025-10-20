#include "render/viewport.hpp"
#include "render/vulkan_types.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include "window/platform_window.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#pragma once

namespace dirk {

class Renderer;
class DirkEngine;

typedef std::uint16_t WindowId;

struct WindowCreateInfo {
    std::string_view title;
    std::uint32_t width, height;
};

/**
 * Engine level abstration that handles converting platform level
 * windows to engine systems
 */
class Window {
public:
    Window(const WindowCreateInfo& createInfo);

    void addViewport(std::shared_ptr<Viewport> inViewport);
    void removeViewport(std::shared_ptr<Viewport> inViewport);

    // rendering interface
    vk::SubmitInfo render();
    vk::PresentInfoKHR present();
    void resize(vk::Extent2D inSize);

    vk::Extent2D getSize() const;
    bool isMinimized() const;
    bool shouldClose() const;

    void processPlatformEvents();

private:
    std::unique_ptr<Platform::PlatformWindow> platformWindow;

    vk::SurfaceKHR surface;
    vk::SwapchainKHR swapChain;
    std::vector<SwapChainImage> swapChainImages;

    vk::Semaphore imageAvailableSemaphore;
    vk::Semaphore renderFinishedSemaphore;

    vk::Format swapChainImageFormat;
    vk::Extent2D swapChainExtent;

    std::uint32_t currentFrame;

    vk::CommandBuffer commandBuffer;

    vk::RenderPass renderPass;

    std::vector<std::shared_ptr<Viewport>> viewports;
};

} // namespace dirk
