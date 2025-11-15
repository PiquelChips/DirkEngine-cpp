#include "platform/window.hpp"
#include "asserts.hpp"
#include "common.hpp"
#include "platform/platform.hpp"
#include <algorithm>
#include <memory>

#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"
#include "vulkan/vulkan_enums.hpp"

#include <cstdint>

namespace dirk::Platform {

Window::Window(const WindowCreateInfo& createInfo, Platform& platform, std::unique_ptr<PlatformWindowImpl> impl)
    : platform(platform), platformWindow(std::move(impl)) {
    surface = platformWindow->getVulkanSurface(gEngine->getRenderer()->getResources().instance);

    SwapChainCreateInfo swapChainInfo{
        .swapChain = swapchain,
        .swapChainImageFormat = swapChainImageFormat,
        .swapChainExtent = swapChainExtent,
        .surface = surface,
        .windowSize = platformWindow->getSize()
    };
    swapChainImages = gEngine->getRenderer()->createSwapChain(swapChainInfo);
}

vk::Extent2D Window::getSize() const { return platformWindow->getSize(); }

void Window::setSize(vk::Extent2D inSize) {
    this->platformWindow->setSize(inSize);

    auto renderer = gEngine->getRenderer();
    auto device = renderer->getResources().device;

    for (auto image : swapChainImages) {
        device.destroyImageView(image);
    }

    device.destroySwapchainKHR(swapchain);

    SwapChainCreateInfo swapChainInfo{
        .swapChain = swapchain,
        .swapChainImageFormat = swapChainImageFormat,
        .swapChainExtent = swapChainExtent,
        .surface = surface,
        .windowSize = platformWindow->getSize()
    };
    swapChainImages = renderer->createSwapChain(swapChainInfo);
}

glm::vec2 Window::getPosition() const { return platformWindow->getPosition(); }
void Window::setPosition(const glm::vec2& inPosition) { platformWindow->setPosition(inPosition); }
std::string_view Window::getTitle() { return platformWindow->getTitle(); }
void Window::setTitle(std::string_view inTitle) { platformWindow->setTitle(inTitle); }

bool Window::isFocused() { return platformWindow->isFocused(); }
bool Window::isMinimized() { return platformWindow->isMinimized(); }

// TODO: handle visibility
void Window::updateVisibility(bool inVisible) {}

vk::SurfaceKHR Window::getVulkanSurface(vk::Instance instance) {
    this->surface = platformWindow->getVulkanSurface(instance);
    return this->surface;
}

vk::SubmitInfo Window::render(ImDrawData* drawData) {
    auto resources = gEngine->getRenderer()->getResources();
    auto result = resources.device.acquireNextImageKHR(swapchain, UINT64_MAX, imageAvailableSemaphore, nullptr);
    checkVulkan(result.result);
    imageIndex = result.value;
    auto image = swapChainImages[imageIndex];

    commandBuffer.reset();

    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
    beginInfo.pInheritanceInfo = nullptr;

    checkVulkan(commandBuffer.begin(&beginInfo));

    vk::RenderingAttachmentInfo colorAttachment{};
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.imageLayout = vk::ImageLayout::eUndefined;
    colorAttachment.resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachment.resolveMode = vk::ResolveModeFlagBits::eAverage; // TODO: what is this?
    colorAttachment.clearValue = vk::ClearColorValue(0.f, 0.f, 0.f, 1.f);

    vk::RenderingAttachmentInfo depthAttachment{};
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.imageLayout = vk::ImageLayout::eUndefined;
    depthAttachment.resolveImageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depthAttachment.resolveMode = vk::ResolveModeFlagBits::eAverage; // TODO: what is this?
    depthAttachment.clearValue = vk::ClearDepthStencilValue(1.f, 0.f);

    vk::RenderingInfo renderInfo{};
    renderInfo.renderArea.offset = vk::Offset2D(0, 0);
    renderInfo.renderArea.extent = swapChainExtent;

    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &colorAttachment;
    renderInfo.pDepthAttachment = &depthAttachment;
    renderInfo.layerCount = 1;

    commandBuffer.beginRendering(renderInfo);

    ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);

    commandBuffer.endRendering();
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
