#include "render/renderer.hpp"
#include "render/renderer_types.hpp"
#include "vulkan/vulkan.hpp"

DEFINE_LOG_CATEGORY(LogRenderer)

RendererFeatures& Renderer::getFeatures() { return features; }

Renderer* createRenderer(RendererCreateInfo& createInfo) {
    switch (createInfo.api) {
    case VulkanApi:
        return new VulkanRenderer(createInfo);
    }

    DIRK_LOG(LogRenderer, FATAL, "an invalid api was specified in RendererCreateInfo.api");
    return nullptr;
}
