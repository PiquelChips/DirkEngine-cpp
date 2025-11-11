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

Window::Window(const WindowCreateInfo& createInfo, Platform* platform, std::unique_ptr<PlatformWindowImpl> impl)
    : platform(platform), platformWindow(std::move(impl)) {
    auto renderer = gEngine->getRenderer();
    surface = platformWindow->getVulkanSurface(renderer->getResources().instance);

    SwapChainCreateInfo swapChainInfo{
        .swapChain = swapchain,
        .swapChainImageFormat = swapChainImageFormat,
        .swapChainExtent = size,
        .renderPass = renderPass,
        .surface = surface,
        .windowSize = platformWindow->getFramebufferSize()
    };
    swapChainImages = renderer->createSwapChain(swapChainInfo);
}

vk::Extent2D Window::getSize() const {
    return platformWindow->getSize();
}

void Window::setSize(vk::Extent2D inSize) {
    this->platformWindow->setSize(inSize);
    this->size = inSize;

    auto renderer = gEngine->getRenderer();
    auto device = renderer->getResources().device;

    for (auto image : swapChainImages) {
        device.destroyFramebuffer(image.frameBuffer);
        device.destroyImageView(image.imageView);
    }

    device.destroySwapchainKHR(swapchain);

    SwapChainCreateInfo swapChainInfo{
        .swapChain = swapchain,
        .swapChainImageFormat = swapChainImageFormat,
        .swapChainExtent = size,
        .renderPass = renderPass,
        .surface = surface,
        .windowSize = platformWindow->getFramebufferSize()
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

    vk::RenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = vk::StructureType::eRenderPassBeginInfo;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = image.frameBuffer;

    // make sure to render on the entire screen
    renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
    renderPassInfo.renderArea.extent = size;

    // clear color is black with 100% opacity
    std::array<vk::ClearValue, 2> clearValues = { vk::ClearColorValue(0.f, 0.f, 0.f, 1.f), vk::ClearDepthStencilValue(1.f, 0.f) };
    renderPassInfo.clearValueCount = clearValues.size();
    renderPassInfo.pClearValues = clearValues.data();

    commandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

    ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);

    commandBuffer.endRenderPass();
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
