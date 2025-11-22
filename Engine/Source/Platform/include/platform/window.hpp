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
    vk::Extent2D size = { 550, 680 };

    bool focused;
    bool visible;
    bool decorated;
    bool floating;
};

// interface for all platform windows
class PlatformWindowImpl {
public:
    virtual ~PlatformWindowImpl() = default;

    virtual void show() = 0;
    virtual void hide() = 0;

    virtual vk::SurfaceKHR getVulkanSurface(vk::Instance instance) = 0;
    virtual void createVulkanSurface(VkInstance instance, VkSurfaceKHR* surface) = 0;
    virtual void* getPlatformHandle() = 0;

    virtual vk::Extent2D getSize() = 0;
    virtual void setSize(vk::Extent2D inSize) = 0;

    virtual glm::vec2 getPosition() = 0;
    virtual void setPosition(const glm::vec2 inPosition) = 0;

    virtual std::string_view getTitle() = 0;
    virtual void setTitle(std::string_view inTitle) = 0;

    virtual bool isFocused() = 0;
    virtual void focus() = 0;

    virtual bool isDecorated() = 0;
    virtual void setDecorated(bool inDecorated) = 0;
};

/**
 * Engine level abstration that handles converting platform level
 * windows to engine systems
 */
class Window {
public:
    Window(const WindowCreateInfo& createInfo, Platform& platform, std::unique_ptr<PlatformWindowImpl> impl);

    void show() { platformWindow->show(); }
    void hide() { platformWindow->hide(); }

    vk::Extent2D getSize() { return platformWindow->getSize(); }
    void setSize(vk::Extent2D inSize) { platformWindow->setSize(inSize); }

    glm::vec2 getPosition() { return platformWindow->getPosition(); }
    void setPosition(const glm::vec2 inPosition) { return platformWindow->setPosition(inPosition); }

    std::string_view getTitle() { return platformWindow->getTitle(); }
    void setTitle(std::string_view inTitle) { platformWindow->setTitle(inTitle); }

    bool isFocused() { return platformWindow->isFocused(); }
    void focus() { platformWindow->focus(); }

    bool isDecorated();
    void setDecorated(bool inDecorated);

    uint32_t getImageCount() { return swapChainImages.size(); } // TODO: what?

    PlatformWindowImpl& getPlatformImpl() { return *platformWindow.get(); }
    void* getPlatformHandle() { return platformWindow->getPlatformHandle(); }
    vk::SurfaceKHR getVulkanSurface() { return surface; }

    vk::SubmitInfo render(ImDrawData* drawData);
    vk::PresentInfoKHR present();

private:
    // render resources
    vk::SwapchainKHR swapchain;
    vk::SurfaceKHR surface;
    vk::CommandBuffer commandBuffer;

    // renderer settings
    vk::Extent2D swapChainExtent;
    vk::SurfaceFormatKHR surfaceFormat;
    vk::PresentModeKHR presentMode;

    vk::Semaphore imageAvailableSemaphore;
    vk::Semaphore renderFinishedSemaphore;
    std::vector<vk::ImageView> swapChainImages;

    // state
    std::uint32_t imageIndex;

    Platform& platform;
    std::unique_ptr<PlatformWindowImpl> platformWindow;
};

} // namespace dirk::Platform
