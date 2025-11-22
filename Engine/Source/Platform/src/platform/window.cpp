#include "platform/window.hpp"
#include "asserts.hpp"
#include "common.hpp"
#include "platform/platform.hpp"

#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"

#include <cstdint>
#include <memory>

namespace dirk::Platform {

Window::Window(const WindowCreateInfo& createInfo, Platform& platform, std::unique_ptr<PlatformWindowImpl> impl)
    : platform(platform), platformWindow(std::move(impl)) {

    platformWindow->setOwningWindow(*this);

    auto renderer = gEngine->getRenderer();
    auto resources = renderer->getResources();

    surface = platformWindow->getVulkanSurface(resources.instance);

    auto formats = resources.physicalDevice.getSurfaceFormatsKHR(surface);
    surfaceFormat = renderer->chooseSwapSurfaceFormat(formats);
    auto presentModes = resources.physicalDevice.getSurfacePresentModesKHR(surface);
    presentMode = renderer->chooseSwapPresentMode(presentModes);

    SwapChainCreateInfo swapChainInfo{
        .swapChain = swapchain,
        .swapChainExtent = swapChainExtent,
        .surface = surface,
        .windowSize = platformWindow->getSize(),
        .surfaceFormat = surfaceFormat,
        .presentMode = presentMode
    };
    swapChainImages = gEngine->getRenderer()->createSwapChain(swapChainInfo);

    imageAvailableSemaphore = gEngine->getRenderer()->createSemaphore();
    renderFinishedSemaphore = gEngine->getRenderer()->createSemaphore();
    commandBuffer = gEngine->getRenderer()->createCommandBuffer();
}

void Window::onResize() {
    DIRK_LOG(LogPlatform, DEBUG, "resizing window")
    auto renderer = gEngine->getRenderer();
    auto device = renderer->getResources().device;

    auto oldSwapchain = swapchain;

    SwapChainCreateInfo swapChainInfo{
        .swapChain = swapchain,
        .swapChainExtent = swapChainExtent,
        .surface = surface,
        .windowSize = platformWindow->getSize(),
        .surfaceFormat = surfaceFormat,
        .presentMode = presentMode
    };
    swapChainImages = renderer->createSwapChain(swapChainInfo);

    if (swapchain)
        device.destroySwapchainKHR(oldSwapchain);
}

vk::SubmitInfo Window::render(ImDrawData* drawData) {
    auto renderer = gEngine->getRenderer();
    auto resources = renderer->getResources();
    auto result = resources.device.acquireNextImageKHR(swapchain, UINT64_MAX, imageAvailableSemaphores[frameIndex], nullptr);
    checkVulkan(result.result);
    imageIndex = result.value;
    auto image = swapChainImages[imageIndex];

    commandBuffer.reset();

    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
    beginInfo.pInheritanceInfo = nullptr;

    checkVulkan(commandBuffer.begin(&beginInfo));

    renderer->transitionImageLayout(commandBuffer, image.image, surfaceFormat.format, vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::eColorAttachmentOptimal);

    vk::RenderingAttachmentInfo colorAttachment{};
    colorAttachment.imageView = image.view;
    colorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachment.resolveMode = vk::ResolveModeFlagBits::eNone;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.clearValue = vk::ClearColorValue(0.f, 0.f, 0.f, 1.f);

    vk::RenderingInfo renderInfo{};
    renderInfo.renderArea.offset = vk::Offset2D(0, 0);
    renderInfo.renderArea.extent = swapChainExtent;
    renderInfo.layerCount = 1;
    renderInfo.viewMask = 0;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &colorAttachment;

    commandBuffer.beginRendering(renderInfo);

    ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);

    commandBuffer.endRendering();

    renderer->transitionImageLayout(commandBuffer, image.image, surfaceFormat.format, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR);

    commandBuffer.end();

    vk::SubmitInfo submitInfo{};
    submitInfo.sType = vk::StructureType::eSubmitInfo;

    // wait semaphores
    vk::PipelineStageFlags waitStage{ vk::PipelineStageFlagBits::eColorAttachmentOutput };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = &waitStage;
    // signal semaphores
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

    // command buffers
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    return submitInfo;
}

vk::PresentInfoKHR Window::present() {
    vk::PresentInfoKHR presentInfo{};
    presentInfo.sType = vk::StructureType::ePresentInfoKHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphore;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // only have one swap chain

    return presentInfo;
}

} // namespace dirk::Platform
