#include "glm/glm.hpp"
#include "render/vulkan_types.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"

#include <memory>
#include <string_view>

#pragma once

// representation of a viewport of the engine (essentially a renderer output)
namespace dirk {

typedef std::uint16_t ViewportId;

struct ViewportCreateInfo {
    vk::Extent2D size;
    std::string_view name;
};

class Camera;
class DirkEngine;

class Viewport {
public:
    Viewport(const ViewportCreateInfo& createInfo, DirkEngine* engine);
    ~Viewport();

    std::shared_ptr<Camera> getCamera() { return camera; }
    vk::Extent2D getSize() const { return size; }
    vk::Semaphore getRenderFinishedSemaphore() { return renderFinishedSemaphore; }

    vk::SubmitInfo render();
    void resize(vk::Extent2D inSize);

    static glm::vec2 screenToViewport(glm::vec2 screenPos, vk::Rect2D viewportRegion);
    static glm::vec2 viewportToScreen(glm::vec2 veiwportPos, vk::Rect2D viewportRegion);

private:
    // this will create render pass, pipeline and all associated stuff. this should only be called
    // on resize
    void createRenderResources();

    std::shared_ptr<Camera> camera;

    // render settings
    std::string_view name;
    vk::Extent2D size;

    // for render output
    ImageMemoryView depthImageMemoryView;
    vk::Format depthFormat;
    ImageMemoryView colorImageMemoryView;
    ImageMemoryView outImageMemoryView;
    vk::Sampler outSampler;
    vk::Framebuffer framebuffer;

    // render objects
    vk::RenderPass renderPass;
    vk::Pipeline graphicsPipeline;
    vk::CommandBuffer commandBuffer;
    vk::PipelineLayout pipelineLayout;

    vk::Semaphore renderFinishedSemaphore;

    DirkEngine* engine;
};

} // namespace dirk
