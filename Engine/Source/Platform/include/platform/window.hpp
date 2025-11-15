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

class Platform;

struct WindowCreateInfo {
    std::string_view title;
    vk::Extent2D size;

    bool focused;
    bool visible;
    bool decorated;
    bool floating;
};

// interface for all platform windows
class PlatformWindowImpl {
public:
    virtual ~PlatformWindowImpl() = default;

    virtual vk::SurfaceKHR getVulkanSurface(vk::Instance instance) = 0;
    virtual void* getPlatformHandle() = 0;

    virtual vk::Extent2D getSize() = 0;
    virtual void setSize(vk::Extent2D inSize) = 0;

    virtual glm::vec2 getPosition() = 0;
    virtual void setPosition(const glm::vec2 inPosition) = 0;

    virtual std::string_view getTitle() = 0;
    virtual void setTitle(std::string_view inTitle) = 0;

    virtual bool isFocused() = 0;
    virtual bool isMinimized() = 0;
};

/**
 * Engine level abstration that handles converting platform level
 * windows to engine systems
 */
class Window {
public:
    Window(const WindowCreateInfo& createInfo, Platform& platform, std::unique_ptr<PlatformWindowImpl> impl);

    vk::Extent2D getSize() const;
    void setSize(vk::Extent2D inSize);
    glm::vec2 getPosition() const;
    void setPosition(const glm::vec2& inPosition);
    void setTitle(std::string_view inTitle);
    std::string_view getTitle();

    uint32_t getImageCount() { return swapChainImages.size(); }

    PlatformWindowImpl& getPlatformImpl() { return *platformWindow.get(); }
    void* getPlatformHandle() { return platformWindow->getPlatformHandle(); }

    bool isFocused();
    bool isMinimized();

    void updateVisibility(bool inVisible);

    vk::SurfaceKHR getVulkanSurface(vk::Instance instance);

    vk::SubmitInfo render(ImDrawData* drawData);
    vk::PresentInfoKHR present();

private:
    // render resources
    vk::SwapchainKHR swapchain;
    vk::SurfaceKHR surface;
    vk::CommandBuffer commandBuffer;
    vk::Extent2D swapChainExtent;

    vk::Semaphore imageAvailableSemaphore;
    vk::Semaphore renderFinishedSemaphore;
    std::vector<vk::ImageView> swapChainImages;

    // settings
    vk::SurfaceFormatKHR surfaceFormat;
    vk::PresentModeKHR presentMode;
    vk::Format swapChainImageFormat;

    // state
    std::uint32_t imageIndex;

    Platform& platform;
    std::unique_ptr<PlatformWindowImpl> platformWindow;
};

} // namespace dirk::Platform
