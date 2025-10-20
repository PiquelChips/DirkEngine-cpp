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
    /**
     * Get platform native window handle
     */
    void* getNativeHandle();

    /**
     * Creates the vulkan surface for this window
     */
    vk::SurfaceKHR createVulkanSurface(vk::Instance instance);

    /**
     * Polls all platform events such as resize and input
     */
    void pollEvents();

    /**
     * Sets the event callback function.
     * This will be used downstream to handle platform events
     */
    // TODO: setup event system
    void setEventCallback();

    /**
     * Returns the size of the vulkan framebuffer linked to this window
     */
    vk::Extent2D getFramebufferSize();

    /**
     * Will return true if the operating system has requested
     * to close the window
     */
    bool shouldClose();
};

} // namespace dirk::Platform
