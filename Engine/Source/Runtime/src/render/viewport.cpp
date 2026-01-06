#include "render/viewport.hpp"
#include "asserts.hpp"
#include "core.hpp"
#include "engine/dirkengine.hpp"
#include "engine/world.hpp"
#include "input/events.hpp"
#include "input/keys.hpp"
#include "logging/logging.hpp"
#include "render/camera.hpp"
#include "render/renderer.hpp"

#include "backends/imgui_impl_vulkan.h"
#include "glm/trigonometric.hpp"
#include "imgui.h"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"

#include <memory>

namespace dirk {

DEFINE_LOG_CATEGORY(LogViewport);

Viewport::Viewport(const ViewportCreateInfo& createInfo)
    : world(createInfo.world), size(createInfo.size), name(createInfo.name) {
    camera = std::make_unique<Camera>(
        CameraCreateInfo{
            .positon = { 0.f, 1000.f, 1000.f },
            .forwardDirection = { 0.f, -1.f, -1.f },
            .fov = glm::radians(45.f),
            .nearClip = .1f,
            .farClip = 100000.f,
        },
        *this);

    auto* eventManager = gEngine->getEventManager();
    eventManager->bindMember(this, &Viewport::Event_MouseButton);

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

void Viewport::tick(float deltaTime) {
    camera->tick(deltaTime);
}

Viewport::~Viewport() {
    cleanupRenderResources();
}

void Viewport::createRenderResources() {
    auto renderer = gEngine->getRenderer();
    auto resources = renderer->getResources();
    auto properties = renderer->getProperties();

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

    pipelineLayout = resources.device.createPipelineLayout(pipelineLayoutInfo);

    std::array<vk::Format, 1> colorAttachmentFormats{ properties.surfaceFormat.format };
    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
    pipelineRenderingCreateInfo.colorAttachmentCount = colorAttachmentFormats.size();
    pipelineRenderingCreateInfo.pColorAttachmentFormats = colorAttachmentFormats.data();
    pipelineRenderingCreateInfo.depthAttachmentFormat = properties.depthFormat;

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
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    pipelineInfo.pNext = &pipelineRenderingCreateInfo;

    pipeline = resources.device.createGraphicsPipeline(VK_NULL_HANDLE, pipelineInfo).value;

    // COLOR RESOURCES

    CreateImageMemoryViewInfo colorInfo{
        .width = size.width,
        .height = size.height,
        .format = properties.surfaceFormat.format,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
        .properties = vk::MemoryPropertyFlagBits::eDeviceLocal,
        .numSamples = properties.msaaSamples,
    };
    colorImageMemoryView = renderer->createImageMemoryView(colorInfo);

    // DEPTH RESOURCES

    CreateImageMemoryViewInfo imvInfo{
        .width = size.width,
        .height = size.height,
        .format = properties.depthFormat,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
        .properties = vk::MemoryPropertyFlagBits::eDeviceLocal,
        .imageAspect = vk::ImageAspectFlagBits::eDepth,
        .numSamples = properties.msaaSamples,
    };
    depthImageMemoryView = renderer->createImageMemoryView(imvInfo);

    CreateImageMemoryViewInfo outInfo{
        .width = size.width,
        .height = size.height,
        .format = properties.surfaceFormat.format,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment,
        .properties = vk::MemoryPropertyFlagBits::eDeviceLocal,
        .numSamples = vk::SampleCountFlagBits::e1,
    };
    outImageMemoryView = renderer->createImageMemoryView(outInfo);

    vk::CommandBuffer commandBuffer = renderer->beginSingleTimeCommands();
    renderer->transitionImageLayout(commandBuffer, depthImageMemoryView.image, properties.depthFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    renderer->transitionImageLayout(commandBuffer, outImageMemoryView.image, properties.surfaceFormat.format, vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal);
    renderer->endSingleTimeCommands(commandBuffer, resources.queues.graphicsQueue);

    vk::SamplerCreateInfo samplerInfo{};
    samplerInfo.magFilter = vk::Filter::eLinear;
    samplerInfo.minFilter = vk::Filter::eLinear;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat; // outside image bounds just use border color
    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    samplerInfo.minLod = -1000;
    samplerInfo.maxLod = 1000;
    samplerInfo.maxAnisotropy = 1.0f;
    sampler = resources.device.createSampler(samplerInfo);

    descriptorSet = ImGui_ImplVulkan_AddTexture(sampler, outImageMemoryView.view, (VkImageLayout) vk::ImageLayout::eShaderReadOnlyOptimal);
}

void Viewport::cleanupRenderResources() {
    ImGui_ImplVulkan_RemoveTexture(descriptorSet);
}

vk::SubmitInfo Viewport::render() {
    auto properties = gEngine->getRenderer()->getProperties();
    commandBuffer.reset();

    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.pInheritanceInfo = nullptr;

    checkVulkan(commandBuffer.begin(&beginInfo));

    vk::RenderingAttachmentInfo colorAttachment{};
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachment.imageView = colorImageMemoryView.view;
    colorAttachment.resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachment.resolveMode = vk::ResolveModeFlagBits::eAverage;
    colorAttachment.resolveImageView = outImageMemoryView.view;
    colorAttachment.clearValue = vk::ClearColorValue(0.f, 0.f, 0.f, 1.f);

    vk::RenderingAttachmentInfo depthAttachment{};
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depthAttachment.imageView = depthImageMemoryView.view;
    depthAttachment.clearValue = vk::ClearDepthStencilValue(1.f, 0.f);

    vk::RenderingInfo renderInfo{};
    renderInfo.renderArea.offset = vk::Offset2D(0, 0);
    renderInfo.renderArea.extent = size;

    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &colorAttachment;
    renderInfo.pDepthAttachment = &depthAttachment;
    renderInfo.layerCount = 1;

    commandBuffer.beginRendering(renderInfo);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(size.width);
    viewport.height = static_cast<float>(size.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    commandBuffer.setViewport(0, 1, &viewport);

    // scissor is dynamic
    vk::Rect2D scissor{};
    scissor.offset = vk::Offset2D(0, 0);
    scissor.extent = size;
    commandBuffer.setScissor(0, 1, &scissor);

    for (auto& pair : world->getActors()) {
        pair.second->recordCommandBuffer(commandBuffer, pipelineLayout, camera);
    }

    commandBuffer.endRendering();
    commandBuffer.end();

    vk::SubmitInfo submitInfo{};
    submitInfo.sType = vk::StructureType::eSubmitInfo;

    // wait semaphores
    vk::PipelineStageFlags waitStage{ vk::PipelineStageFlagBits::eColorAttachmentOutput };
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    return submitInfo;
}

void Viewport::renderImGui() {
    ImGuiIO& io = ImGui::GetIO();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin(name.data());

    // handle input
    {
        focused = ImGui::IsWindowFocused();
        hovered = ImGui::IsWindowHovered();
        if (hovered && acceptsInput) {
            camera->addLookInput(io.MouseDelta);
        }

        if (focused && acceptsInput) {
            glm::vec3 move{ 0.f };
            if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
                move += FORWARD_DIRECTION;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_S)) {
                move -= FORWARD_DIRECTION;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_A)) {
                move += LEFT_DIRECTION;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_D)) {
                move -= LEFT_DIRECTION;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
                move += UP_DIRECTION;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_C)) {
                move -= UP_DIRECTION;
            }

            camera->addMoveInput(move);
        }
    }

    resize(ImGui::GetContentRegionAvail());

    ImGui::Image((ImTextureID) (VkDescriptorSet) descriptorSet, size);

    ImGui::End();
    ImGui::PopStyleVar();
}

void Viewport::resize(vk::Extent2D inSize) {
    if ((inSize.width == size.width && inSize.height == size.height) || inSize.width == 0 || inSize.height == 0) {
        return;
    }

    size = inSize;
    camera->resize(size);

    auto device = gEngine->getRenderer()->getResources().device;
    device.waitIdle();

    cleanupRenderResources();
    createRenderResources();
}

bool Viewport::Event_MouseButton(Input::MouseButtonEvent& event) {
    if (event.button == Input::MouseButton::Right) {
        acceptsInput = event.state == Input::KeyState::Pressed;
        return true;
    }
    return false;
}

void Viewport::setWorld(std::shared_ptr<World> inWorld) { world = inWorld; }

} // namespace dirk
