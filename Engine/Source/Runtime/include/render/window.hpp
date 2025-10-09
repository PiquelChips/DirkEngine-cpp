#include "render/viewport.hpp"
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

typedef std::uint16_t WindowId;

struct WindowCreateInfo {
    const std::string& title;
    std::uint32_t width, height;
};

struct ViewportAssignement {
    ViewportId viewportId;
    vk::Rect2D region;
    int renderOrder;
    bool enabled;
};

/**
 * Engine level abstration that handles converting platform level
 * windows to engine systems
 */
class Window {
public:
    // viewport management
    void addViewport(ViewportId id, vk::Rect2D region, int order = 0);
    void removeViewport(ViewportId id);
    void updateViewportRegion(ViewportId id, vk::Rect2D newRegion);
    void setViewportRenderOrder(ViewportId id, int order);
    void setViewportEnabled(ViewportId id, bool enabled);

    bool hasViewport(ViewportId id);
    std::vector<const ViewportAssignement> getViewportAssignements() const;
    ViewportId getViewportAt(glm::vec2 windowCoords) const;

    // rendering interface
    void beginFrame();
    vk::Framebuffer getCurrentFramebuffer();
    uint32_t getCurrentImageIndex();
    void Present();

    vk::Extent2D getSize() const;
    bool isMinimized() const;
    bool shouldClose() const;

    void processPlatformEvents();

private:
    std::unique_ptr<PlatformWindow> platformWindow;

    std::unordered_map<ViewportId, ViewportAssignement> viewportAssignements;

    vk::SwapchainKHR swapchain;
    std::vector<vk::Framebuffer> framebuffers;
    uint32_t currentImageIndex;
};

} // namespace dirk
