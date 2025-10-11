#include "input/keys.hpp"

#include "glm/glm.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"

#include <cstdint>
#include <string>

#pragma once

namespace dirk::Platform {

// interface for all platform windows
class PlatformWindow {
public:
    virtual ~PlatformWindow() = default;

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
    virtual vk::Extent2D getFramebufferSize() = 0;

    /**
     * Will return true if the operating system has requested
     * to close the window
     */
    virtual bool shouldClose() = 0;
};

} // namespace dirk
