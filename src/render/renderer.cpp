#include "render/renderer.hpp"
#include "render/renderer_types.hpp"
#include "vulkan/vulkan.hpp"

namespace dirk {

DEFINE_LOG_CATEGORY(LogRenderer)

RendererCreateInfo::operator RendererProperties() {
    return RendererProperties{
        .applicationName = applicationName,
        .windowWidth = windowWidth,
        .windowHeight = windowHeight,
        .api = api,
        .engine = engine,
    };
};

RendererFeatures& Renderer::getFeatures() noexcept { return features; }
RendererProperties& Renderer::getProperties() noexcept { return properties; };

Renderer* createRenderer(RendererCreateInfo& createInfo) {
    switch (createInfo.api) {
    case Vulkan:
        return new VulkanRenderer(createInfo);
    }

    DIRK_LOG(LogRenderer, FATAL, "an invalid api was specified in RendererCreateInfo.api");
    return nullptr;
}

} // namespace dirk
