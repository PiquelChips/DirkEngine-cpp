#include "render/viewport.hpp"
#include "engine/dirkengine.hpp"
#include "engine/world.hpp"
#include "glm/trigonometric.hpp"
#include "render/camera.hpp"
#include "render/renderer.hpp"
#include "vulkan/vulkan_handles.hpp"
#include <memory>

namespace dirk {

Viewport::Viewport(const ViewportCreateInfo& createInfo)
    : world(createInfo.world) {
    camera = std::make_unique<Camera>(
        CameraCreateInfo{
            .positon = { 0.f, 1000.f, 1000.f },
            .forwardDirection = { 0.f, -1.f, -1.f },
            .fov = glm::radians(45.f),
            .nearClip = .1f,
            .farClip = 100000.f,
        },
        this);

    auto renderer = gEngine->getRenderer();
    auto resources = renderer->getResources();
    auto properties = renderer->getProperties();
    renderFinishedSemaphore = renderer->createSemaphore();

    // COMMAND BUFFER

    vk::CommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
    cmdAllocInfo.commandPool = resources.commandPool;
    cmdAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    cmdAllocInfo.commandBufferCount = 1;
    commandBuffer = resources.device.allocateCommandBuffers(cmdAllocInfo)[0];

    createRenderResources();
}

void Viewport::createRenderResources() {
    auto renderer = gEngine->getRenderer();
    auto resources = renderer->getResources();
    auto properties = renderer->getProperties();

    // RENDER PASS

    vk::AttachmentDescription colorAttachment{};
    colorAttachment.format = properties.swapChainImageFormat;
    colorAttachment.samples = properties.msaaSamples;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::AttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::AttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = properties.msaaSamples;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
    depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = properties.swapChainImageFormat;
    colorAttachmentResolve.samples = vk::SampleCountFlagBits::e1;
    colorAttachmentResolve.loadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachmentResolve.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachmentResolve.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachmentResolve.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass{};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments = &colorAttachmentResolveRef;

    vk::SubpassDependency dependency{};
    // src
    dependency.srcSubpass = vk::SubpassExternal;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    // dst
    dependency.dstSubpass = 0;
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

    std::array attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
    vk::RenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = vk::StructureType::eRenderPassCreateInfo;
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    renderPass = resources.device.createRenderPass(renderPassInfo);

    // GRPAHICS PIPELINE

    vk::ShaderModule vert = renderer->loadShaderModule("shader.vert");
    vk::ShaderModule frag = renderer->loadShaderModule("shader.frag");

    // vert shader
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = vert;
    vertShaderStageInfo.pName = "main"; // entrypoint

    // frag shader
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = frag;
    fragShaderStageInfo.pName = "main"; // entrypoint

    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptons = Vertex::getAttributeDescriptions();
    // vertex input
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = vk::StructureType::ePipelineVertexInputStateCreateInfo;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptons.size();
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptons.data();

    // input assembly
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = vk::StructureType::ePipelineInputAssemblyStateCreateInfo;
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = vk::False;

    // viewport & scissor
    std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

    vk::PipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = vk::StructureType::ePipelineDynamicStateCreateInfo;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    vk::PipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = vk::StructureType::ePipelineViewportStateCreateInfo;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // rasterizer
    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = vk::StructureType::ePipelineRasterizationStateCreateInfo;
    rasterizer.depthClampEnable = vk::False;
    rasterizer.rasterizerDiscardEnable = vk::False;  // enabling this disables output to frame buffer
    rasterizer.polygonMode = vk::PolygonMode::eFill; // fill the polygons with fragments
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
    rasterizer.depthBiasEnable = vk::False;

    // disabled for now
    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = vk::StructureType::ePipelineMultisampleStateCreateInfo;
    multisampling.sampleShadingEnable = vk::False;
    multisampling.rasterizationSamples = properties.msaaSamples;

    // color blending
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = vk::False;

    vk::PipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = vk::StructureType::ePipelineColorBlendStateCreateInfo;
    colorBlending.logicOpEnable = vk::False;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    vk::PipelineDepthStencilStateCreateInfo depthTestInfo{};
    depthTestInfo.depthTestEnable = vk::True;
    depthTestInfo.depthWriteEnable = vk::True;
    depthTestInfo.depthCompareOp = vk::CompareOp::eLess;

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &resources.descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    auto pipelineLayout = resources.device.createPipelineLayout(pipelineLayoutInfo);
    // actually create the graphics pipeline

    vk::GraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = vk::StructureType::eGraphicsPipelineCreateInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    // all the fixed function stages
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthTestInfo;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;

    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    vk::Pipeline pipeline = resources.device.createGraphicsPipeline(VK_NULL_HANDLE, pipelineInfo).value;

    resources.device.destroyShaderModule(vert);
    resources.device.destroyShaderModule(frag);

    // COLOR RESOURCES

    CreateImageMemoryViewInfo colorInfo{
        .width = size.width,
        .height = size.height,
        .format = properties.swapChainImageFormat,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
        .properties = vk::MemoryPropertyFlagBits::eDeviceLocal,
        .numSamples = properties.msaaSamples,
    };
    colorImageMemoryView = renderer->createImageMemoryView(colorInfo);

    // DEPTH RESOURCES

    depthFormat = Renderer::findSupportedFormat(
        resources.physicalDevice,
        { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment);

    CreateImageMemoryViewInfo imvInfo{
        .width = size.width,
        .height = size.height,
        .format = depthFormat,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
        .properties = vk::MemoryPropertyFlagBits::eDeviceLocal,
        .imageAspect = vk::ImageAspectFlagBits::eDepth,
        .numSamples = properties.msaaSamples,
    };
    depthImageMemoryView = renderer->createImageMemoryView(imvInfo);

    vk::CommandBuffer commandBuffer = renderer->beginSingleTimeCommands();
    renderer->transitionImageLayout(commandBuffer, depthImageMemoryView.image, depthFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal, 1);
    renderer->endSingleTimeCommands(commandBuffer, resources.queues.graphicsQueue);

    // TODO: out image memory view (resolve output of render pass)
    // TODO: out image sampler (to be used by ImGUI)
    // make sure textures are rendered to shader read optimal layout

    // FRAME BUFFER

    std::array framebufferAttachments{ colorImageMemoryView.view, depthImageMemoryView.view, outImageMemoryView.view };
    vk::FramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = vk::StructureType::eFramebufferCreateInfo;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = framebufferAttachments.size();
    framebufferInfo.pAttachments = framebufferAttachments.data();
    framebufferInfo.width = size.width;
    framebufferInfo.height = size.height;
    framebufferInfo.layers = 1;
    framebuffer = resources.device.createFramebuffer(framebufferInfo);
}

vk::SubmitInfo Viewport::render() {
    commandBuffer.reset();

    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
    beginInfo.pInheritanceInfo = nullptr;

    checkVulkan(commandBuffer.begin(&beginInfo));

    vk::RenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = vk::StructureType::eRenderPassBeginInfo;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffer;

    // make sure to render on the entire screen
    renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
    renderPassInfo.renderArea.extent = size;

    // clear color is black with 100% opacity
    std::array<vk::ClearValue, 2> clearValues = { vk::ClearColorValue(0.f, 0.f, 0.f, 1.f), vk::ClearDepthStencilValue(1.f, 0.f) };
    renderPassInfo.clearValueCount = clearValues.size();
    renderPassInfo.pClearValues = clearValues.data();

    commandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

    for (auto pair : world->getActors()) {
        pair.second->recordCommandBuffer(commandBuffer, pipelineLayout, camera);
    }

    commandBuffer.endRenderPass();
    commandBuffer.end();

    vk::SubmitInfo submitInfo{};
    submitInfo.sType = vk::StructureType::eSubmitInfo;

    // wait semaphores
    vk::PipelineStageFlags waitStage{ vk::PipelineStageFlagBits::eColorAttachmentOutput };
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = &waitStage;
    // signal semaphores
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

    // command buffers
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    return submitInfo;
}

// TODO: resize viewport
void Viewport::resize(vk::Extent2D inSize) {}

void Viewport::setWorld(std::shared_ptr<World> inWorld) { world = inWorld; }

} // namespace dirk
