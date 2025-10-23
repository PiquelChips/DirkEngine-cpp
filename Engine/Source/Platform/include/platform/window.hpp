#include "input/keys.hpp"

#include "glm/glm.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"

#include <cstdint>
#include <memory>
#include <string_view>

#pragma once

namespace dirk {

struct SwapChainImage {
    vk::ImageView imageView;
    vk::Framebuffer frameBuffer;

    operator bool() const { return imageView && frameBuffer; }
};

namespace Platform {

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

    Platform* platform;
};

/**
 * Engine level abstration that handles converting platform level
 * windows to engine systems
 */
class Window {
public:
    Window(const WindowCreateInfo& createInfo);

    vk::Extent2D getSize() const;
    void setSize(vk::Extent2D inSize);
    glm::vec2 getPosition() const;
    void setPosition(const glm::vec2& inPosition);
    void setTitle(std::string_view inTitle);
    std::string_view getTitle();

    uint32_t getImageCount(); // swap chain image count
    vk::RenderPass getRenderpass();

    void* getPlatformHandle();

    bool isFocused();
    bool isMinimized();

    void updateVisibility(bool inVisible);

    vk::SurfaceKHR createSurface(vk::Instance instance);

private:
    Platform* platform;
    std::unique_ptr<PlatformWindowImpl> platformWindow;

    // vulkan stuff
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

} // namespace Platform

} // namespace dirk
