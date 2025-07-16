#include "render/renderer.hpp"
#include "render/renderer_types.hpp"
#include "vulkan/vulkan.hpp"

DEFINE_LOG_CATEGORY(LogRenderer)

namespace dirk {

RendererFeatures& Renderer::getFeatures() noexcept { return features; }
RendererProperties& Renderer::getProperties() noexcept { return properties; };

Renderer* createRenderer(RendererCreateInfo& createInfo) {
    switch (createInfo.api) {
    case VulkanApi:
        return new VulkanRenderer(createInfo);
    }

    DIRK_LOG(LogRenderer, FATAL, "an invalid api was specified in RendererCreateInfo.api");
    return nullptr;
}

} // namespace dirk
