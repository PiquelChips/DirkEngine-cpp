#include "common.hpp"
#include "imgui.h"
#include "input/keys.hpp"

#include "glm/glm.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"

#include <cstdint>
#include <memory>
#include <string_view>

#pragma once

namespace dirk::Platform {

class Window;
class PlatformWindowImpl;
class Platform;

struct WindowCreateInfo {
    std::string_view title;
    vk::Extent2D size;

    bool focused;
    bool visible;
    bool decorated;
    bool floating;
};

/**
 * Engine level abstration that handles converting platform level
 * windows to engine systems
 */
class Window {
public:
    Window(const WindowCreateInfo& createInfo, Platform* platform);

    vk::Extent2D getSize() const;
    void setSize(vk::Extent2D inSize);
    glm::vec2 getPosition() const;
    void setPosition(const glm::vec2& inPosition);
    void setTitle(std::string_view inTitle);
    std::string_view getTitle();

    uint32_t getImageCount() { return swapChainImages.size(); }
    vk::RenderPass getRenderpass() { return renderPass; }

    void* getPlatformHandle();

    bool isFocused();
    bool isMinimized();

    void updateVisibility(bool inVisible);

    vk::SurfaceKHR createSurface(vk::Instance instance);

    vk::SubmitInfo render(ImDrawData* drawData);
    vk::PresentInfoKHR present();

private:
    // render resources
    vk::SwapchainKHR swapchain;
    vk::SurfaceKHR surface;
    vk::RenderPass renderPass;
    vk::CommandBuffer commandBuffer;

    vk::Semaphore imageAvailableSemaphore;
    vk::Semaphore renderFinishedSemaphore;
    std::vector<SwapChainImage> swapChainImages;

    // settings
    vk::Extent2D size;
    vk::SurfaceFormatKHR surfaceFormat;
    vk::PresentModeKHR presentMode;
    vk::Format swapChainImageFormat;

    // state
    std::uint32_t imageIndex;

    Platform* platform;
    std::unique_ptr<PlatformWindowImpl> platformWindow;
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
    virtual vk::Extent2D getFramebufferSize() = 0;

    /**
     * Will return true if the operating system has requested
     * to close the window
     */
    virtual bool shouldClose() = 0;
};

} // namespace dirk::Platform
