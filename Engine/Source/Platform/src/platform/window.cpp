#include "platform/window.hpp"

namespace dirk::Platform {

Window::Window(const WindowCreateInfo& createInfo) {
    // TODO: create platform window
    auto renderer = gEngine->getRenderer();
    surface = platformWindow->createVulkanSurface(renderer->getResources().instance);

    SwapChainCreateInfo swapChainInfo{
        .swapChain = swapChain,
        .swapChainImageFormat = swapChainImageFormat,
        .swapChainExtent = swapChainExtent,
        .renderPass = renderPass,
        .surface = surface,
        .windowSize = platformWindow->getFramebufferSize()
    };
    swapChainImages = renderer->createSwapChain(swapChainInfo);
}

vk::Extent2D Window::getSize() const {}
void Window::setSize(vk::Extent2D inSize) {}
glm::vec2 Window::getPosition() const {}
void Window::setPosition(const glm::vec2& inPosition) {}
void Window::setTitle(std::string_view inTitle) {}
std::string_view Window::getTitle() {}

void* Window::getPlatformHandle() {}

bool Window::isFocused() {}
bool Window::isMinimized() {}

void Window::updateVisibility(bool inVisible) {}

vk::SurfaceKHR Window::createSurface(vk::Instance instance) {}

/**
vk::SubmitInfo Window::render() {
    // TODO: render window UI with ImGUI

    std::vector<vk::Semaphore> waitSemaphores(viewports.size());
    for (auto& viewport : viewports) {
        waitSemaphores.emplace_back(viewport->getRenderFinishedSemaphore());
    }

    vk::SubmitInfo submitInfo{};
    submitInfo.sType = vk::StructureType::eSubmitInfo;

    // wait semaphores
    vk::PipelineStageFlags waitStage{ vk::PipelineStageFlagBits::eColorAttachmentOutput };
    submitInfo.waitSemaphoreCount = waitSemaphores.size();
    submitInfo.pWaitSemaphores = waitSemaphores.data();
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
    auto renderer = gEngine->getRenderer();
    auto device = renderer->getResources().device;

    // acquire image from swapChain
    auto imageIndex = device.acquireNextImageKHR(swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE).value;

    vk::PresentInfoKHR presentInfo{};
    presentInfo.sType = vk::StructureType::ePresentInfoKHR;
    // make sure to wait for the image to be rendered
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphore;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // only have one swap chain

    currentFrame = (++currentFrame) % swapChainImages.size();

    return presentInfo;
}
*/

} // namespace dirk::Platform
