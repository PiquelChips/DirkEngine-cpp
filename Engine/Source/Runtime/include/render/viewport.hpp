#include "glm/glm.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"

#include "common.hpp"

#include <memory>
#include <string_view>

#pragma once

// representation of a viewport of the engine (essentially a renderer output)
namespace dirk {

typedef std::uint16_t ViewportId;

class Camera;
class DirkEngine;
class World;

struct ViewportCreateInfo {
    std::string_view name;
    vk::Extent2D size;
    std::shared_ptr<World> world;
};

class Viewport {
public:
    Viewport(const ViewportCreateInfo& createInfo);
    ~Viewport();

    void setWorld(std::shared_ptr<World> inWorld);
    std::unique_ptr<Camera>& getCamera() { return camera; }
    vk::Extent2D getSize() const { return size; }
    vk::Semaphore getRenderFinishedSemaphore() { return renderFinishedSemaphore; }

    vk::SubmitInfo render();
    void renderImGui();
    void resize(vk::Extent2D inSize) { size = inSize; }

private:
    // this will create render pass, pipeline and all associated stuff. this should only be called
    // on resize
    void createRenderResources();

    std::unique_ptr<Camera> camera;

    // render settings
    std::string_view name;
    vk::Extent2D size;

    // for render output
    ImageMemoryView depthImageMemoryView;
    ImageMemoryView colorImageMemoryView;
    ImageMemoryView outImageMemoryView;
    vk::Sampler sampler;
    vk::DescriptorSet descriptorSet;

    // render objects
    vk::Pipeline pipeline;
    vk::CommandBuffer commandBuffer;
    vk::PipelineLayout pipelineLayout;

    vk::Semaphore renderFinishedSemaphore;

    std::shared_ptr<World> world;
};

} // namespace dirk
