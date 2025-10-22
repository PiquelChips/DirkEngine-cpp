#include "input/keys.hpp"

#include "glm/glm.hpp"
#include "vulkan/vulkan.hpp"

#include <memory>

#pragma once

namespace dirk::Platform {

class Window;
class PlatformWindowImpl;

struct SwapChainImage {
    vk::ImageView imageView;
    vk::Framebuffer frameBuffer;

    operator bool() const { return imageView && frameBuffer; }
};

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

    // rendering interface
    vk::SubmitInfo render();
    vk::PresentInfoKHR present();
    void resize(vk::Extent2D inSize);

    vk::Extent2D getSize() const;
    glm::vec2 getPosition() const;
    bool isMinimized() const;
    bool shouldClose() const;

    void processPlatformEvents();

private:
    std::unique_ptr<Platform::PlatformWindowImpl> platformWindow;

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
};

// interface for all platform windows
class PlatformWindowImpl {
public:
    virtual ~PlatformWindowImpl() = default;
    /**
     * Get platform native window handle
     */
    virtual void* getNativeHandle() = 0;

    /**
     * Creates the vulkan surface for this window
     */
    virtual vk::SurfaceKHR createVulkanSurface(vk::Instance instance) = 0;

    /**
     * Polls all platform events such as resize and input
     */
    virtual void pollEvents() = 0;

    /**
     * Sets the event callback function.
     * This will be used downstream to handle platform events
     */
    // TODO: setup event system
    virtual void setEventCallback() = 0;

    /**
     * Returns the size of the vulkan framebuffer linked to this window
     */
    virtual vk::Extent2D getSize() = 0;

    /**
     * Will return true if the operating system has requested
     * to close the window
     */
    virtual bool shouldClose() = 0;
};

} // namespace dirk::Platform
