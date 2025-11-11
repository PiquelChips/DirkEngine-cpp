#ifdef PLATFORM_LINUX

#pragma once

#include "platform/linux/linux.hpp"
#include "platform/platform.hpp"
#include "platform/window.hpp"

#include "vulkan/vulkan_handles.hpp"
#include "wayland-client-core.h"

namespace dirk::Platform::Linux {

struct LinuxWindowCreateInfo {
    LinuxPlatform* platformImpl;
    const WindowCreateInfo& createInfo;
};

class LinuxWindow : public PlatformWindowImpl {
public:
    LinuxWindow(const LinuxWindowCreateInfo& createInfo);
    ~LinuxWindow();

    vk::SurfaceKHR getVulkanSurface(vk::Instance instance) override;
    void* getPlatformHandle() override { return wlSurface; }

    vk::Extent2D getSize() override;
    void setSize(vk::Extent2D inSize) override;
    vk::Extent2D getFramebufferSize() override;

    glm::vec2 getPosition() override;
    void setPosition(const glm::vec2 inPosition) override;

    std::string_view getTitle() override;
    void setTitle(std::string_view inTitle) override;

    bool isFocused() override;
    bool isMinimized() override;

private:
    LinuxPlatform* linuxPlatform;

    wl_surface* wlSurface;
    vk::SurfaceKHR vkSurface;
};

} // namespace dirk::Platform::Linux

#endif
