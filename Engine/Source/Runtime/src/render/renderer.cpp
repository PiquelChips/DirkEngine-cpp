#include "render/renderer.hpp"
#include "core/asserts.hpp"
#include "core/globals.hpp"
#include "engine/dirkengine.hpp"
#include "engine/world.hpp"
#include "render/camera.hpp"
#include "render/viewport.hpp"
#include "render/vulkan_types.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "resources/resource_manager.hpp"
#include "tinygltf.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

namespace dirk {

DEFINE_LOG_CATEGORY(LogVulkan)
DEFINE_LOG_CATEGORY(LogVulkanValidation)
DEFINE_LOG_CATEGORY(LogRenderer)

Renderer::Renderer(const RendererCreateInfo& createInfo) {
    DIRK_LOG(LogVulkan, INFO, "initlializing Vulkan...");
    this->instance = createVulkanInstance();
    if (!this->instance) {
        DIRK_LOG(LogVulkan, FATAL, "instance creation failed");
    }
#ifdef ENABLE_VALIDATION_LAYERS
    this->debugMessenger = setupDebugMessenger();
    if (!this->debugMessenger)
        DIRK_LOG(LogVulkan, ERROR, "failed to create vulkan layer validation debug messenger");
#endif
    this->physicalDevice = selectPhysicalDevice();
    if (!this->physicalDevice) {
        DIRK_LOG(LogVulkan, FATAL, "failed to get a physical device");
    }

    this->device = createLogicalDevice();
    if (!this->device) {
        DIRK_LOG(LogVulkan, FATAL, "failed to create logical device");
    }

    this->queues = createQueues();

    std::vector<vk::Image> swapChainImages = createSwapChain();

    this->commandPool = createCommandPool();
    if (!this->commandPool) {
        DIRK_LOG(LogVulkan, FATAL, "failed to create command pool");
    }

    this->descriptorSetLayout = createDescriptorSetLayout();
    if (!this->descriptorSetLayout) {
        DIRK_LOG(LogVulkan, FATAL, "failed to create descriptor set layout");
    }

    this->descriptorPool = createDescriptorPool();
    if (!this->descriptorPool) {
        DIRK_LOG(LogVulkan, FATAL, "failed to create descriptor pool");
    }

    this->colorImageMemoryView = createColorResources();
    if (!this->colorImageMemoryView) {
        DIRK_LOG(LogVulkan, FATAL, "failed to create color image");
    }

    this->depthImageMemoryView = createDepthResources();
    if (!this->depthImageMemoryView) {
        DIRK_LOG(LogVulkan, FATAL, "failed to create depth image");
    }

    this->renderPass = createRenderPass();
    if (!this->renderPass) {
        DIRK_LOG(LogVulkan, FATAL, "failed to create render pass");
    }

    this->graphicsPipeline = createGraphicsPipeline();
    if (!this->graphicsPipeline) {
        DIRK_LOG(LogVulkan, FATAL, "failed to create graphics pipeline");
    }

    this->swapChainImages = createSwapChainImages(swapChainImages);
    this->inFlightImages = createInFlightImages(MAX_FRAMES_IN_FLIGHT);

    DIRK_LOG(LogVulkan, INFO, "vulkan initialized successfully");
}

Renderer::~Renderer() {
    // make sure all device ops are finished
    device.waitIdle();
    DIRK_LOG(LogVulkan, INFO, "cleaning up renderer");
}

void Renderer::renderFrame() {
    InFlightImage image = inFlightImages[currentFrame];
    check(image);

    auto [imageAvailableSemaphore, renderFinishedSemaphore] = semaphores[currentSemaphore];

    // wait for previous frame
    checkVulkan(device.waitForFences(1, &image.inFlightFence, vk::True, UINT64_MAX));

    // acquire image from swapChain
    auto [result, imageIndex] = device.acquireNextImageKHR(swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE);
    if (result == vk::Result::eErrorOutOfDateKHR) {
        recreateSwapChain();
        return;
    }

    // only reset after or we risk blocking with an unsignalled fence
    checkVulkan(device.resetFences(1, &image.inFlightFence));

    // record the command buffer
    image.commandBuffer.reset();
    recordCommandBuffer(image.commandBuffer, imageIndex);

    // submit the command buffer
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
    submitInfo.pCommandBuffers = &image.commandBuffer;

    checkVulkan(queues.graphicsQueue.submit(1, &submitInfo, image.inFlightFence));

    vk::PresentInfoKHR presentInfo{};
    presentInfo.sType = vk::StructureType::ePresentInfoKHR;

    // make sure to wait for the image to be rendered
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphore;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // only have one swap chain

    result = queues.presentQueue.presentKHR(&presentInfo);
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapChain();
        return;
    }

    currentFrame = (++currentFrame) % MAX_FRAMES_IN_FLIGHT;
    currentSemaphore = (++currentSemaphore) % semaphores.size();
}

ViewportId Renderer::createViewport(const ViewportCreateInfo& createInfo) {
    // TODO: properly assign viewport IDs
    ViewportId id = viewports.size();
    viewports[id] = std::make_unique<Viewport>(createInfo);
    return id;
}

void Renderer::destroyViewport(ViewportId id) {
    viewports.erase(id);
}

vk::Instance Renderer::createVulkanInstance() {
    // vk app
    vk::ApplicationInfo appInfo{};
    appInfo.sType = vk::StructureType::eApplicationInfo;
    appInfo.pApplicationName = "DirkEngine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "DirkEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = vk::ApiVersion14;

    auto instanceExtensions = getRequiredInstanceExtensions();
    if (!checkRequiredInstanceExtensions(instanceExtensions))
        return nullptr;

    vk::InstanceCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eInstanceCreateInfo;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = instanceExtensions.size();
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();

#ifdef ENABLE_VALIDATION_LAYERS
    check(checkValidationLayerSupport());
    DIRK_LOG(LogVulkan, INFO, "using validation layers");
    createInfo.enabledLayerCount = validationLayers.size();
    createInfo.ppEnabledLayerNames = validationLayers.data();
#else
    createInfo.enabledLayerCount = 0;
#endif

    return vk::createInstance(createInfo);
}

std::vector<const char*> Renderer::getRequiredInstanceExtensions() {
    // TODO: get required instance extensions from platform
    /**
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#ifdef ENABLE_VALIDATION_LAYERS
    extensions.push_back(vk::EXTDebugUtilsExtensionName);
#endif

    return extensions;
    */
    return std::vector<const char*>();
}

bool Renderer::checkRequiredInstanceExtensions(std::vector<const char*>& extensions) {
    auto availableExtensions = vk::enumerateInstanceExtensionProperties();
    for (const char* extensionName : extensions) {
        bool layerFound = false;

        for (const auto& extensionProperties : availableExtensions)
            if (strcmp(extensionName, extensionProperties.extensionName) == 0)
                layerFound = true;

        if (!layerFound) {
            DIRK_LOG(LogVulkan, FATAL, "instance extension \"" << extensionName << "\" not found");
            return false;
        }
    }

    return true;
}

vk::PhysicalDevice Renderer::selectPhysicalDevice() {
    auto devices = instance.enumeratePhysicalDevices();
    vk::PhysicalDevice physicalDevice;

    // rank each available device
    std::multimap<int, vk::PhysicalDevice> candidates;

    for (const auto& device : devices) {
        int score = getDeviceSuitability(device);
        candidates.insert(std::make_pair(score, device));
    }

    if (candidates.rbegin()->first > 0) {
        physicalDevice = candidates.rbegin()->second;
        msaaSamples = Renderer::getMaxUsableSampleCount(physicalDevice);
    } else {
        return nullptr;
    }

    vk::PhysicalDeviceProperties deviceProperties = physicalDevice.getProperties();
    // TODO: get more human readable data (like enum values)
    DIRK_LOG(LogVulkan, INFO,
             "physical device selected: "
                 << "\n\tvendor id: " << deviceProperties.vendorID
                 << "\n\tdevice id: " << deviceProperties.deviceID
                 << "\n\tdevice name: " << deviceProperties.deviceName
                 //<< "\n\tdevice type: " << deviceProperties.deviceType
                 << "\n\tapi version: " << deviceProperties.apiVersion
                 << "\n\tdriver version: " << deviceProperties.driverVersion);

    this->features = Renderer::getRendererFeatures(physicalDevice);

    return physicalDevice;
}

int Renderer::getDeviceSuitability(vk::PhysicalDevice device) {
    check(device);

    vk::PhysicalDeviceProperties deviceProperties = device.getProperties();
    vk::PhysicalDeviceFeatures deviceFeatures = device.getFeatures();

    // TODO: update with vulkan tutorial checks

    DIRK_LOG(LogVulkan, DEBUG, "found device: " << deviceProperties.deviceName);

    // prereturn required stuff
    if (!deviceFeatures.geometryShader)
        return 0;

    QueueFamilyIndices indices = Renderer::findQueueFamilies(device);
    if (!indices.isComplete())
        return 0;

    if (!checkDeviceExtensionSupport(device))
        return 0;

    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
    if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty())
        return 0;

    // calculate a score to create preference based on device
    int score = 0;

    if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
        score += 1000;

    score += deviceProperties.limits.maxImageDimension2D;

    if (indices.presentFamily == indices.graphicsFamily)
        score += 10;

    score += swapChainSupport.formats.size();
    score += swapChainSupport.presentModes.size();

    score += Renderer::getRendererFeatures(device).getScore();

    return score;
}

bool Renderer::checkDeviceExtensionSupport(vk::PhysicalDevice device) {
    std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties();

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

SwapChainSupportDetails Renderer::querySwapChainSupport(vk::PhysicalDevice device) {
    return SwapChainSupportDetails{
        .capabilities = device.getSurfaceCapabilitiesKHR(surface),
        .formats = device.getSurfaceFormatsKHR(surface),
        .presentModes = device.getSurfacePresentModesKHR(surface),
    };
}

vk::Device Renderer::createLogicalDevice() {
    QueueFamilyIndices indices = Renderer::findQueueFamilies(physicalDevice);

    // queues
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos(uniqueQueueFamilies.size());
    float queuePriority = 1.f;
    for (int i = 0; i < uniqueQueueFamilies.size(); i++) {
        std::set<uint32_t>::iterator iter = uniqueQueueFamilies.find(i);
        if (iter == uniqueQueueFamilies.end())
            return nullptr;

        uint32_t queueFamily = *iter;

        vk::DeviceQueueCreateInfo queueCreateInfo;
        queueCreateInfo.sType = vk::StructureType::eDeviceQueueCreateInfo;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfos[i] = queueCreateInfo;
    }

    vk::PhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = vk::True;

    vk::DeviceCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eDeviceCreateInfo;
    createInfo.pEnabledFeatures = &deviceFeatures;
    // queues
    createInfo.queueCreateInfoCount = queueCreateInfos.size();
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    // extensions
    createInfo.enabledExtensionCount = deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    return physicalDevice.createDevice(createInfo);
}

Queues Renderer::createQueues() {
    Queues queues;

    QueueFamilyIndices indices = Renderer::findQueueFamilies(physicalDevice);
    queues.graphicsQueue = device.getQueue(indices.graphicsFamily.value(), 0);
    queues.presentQueue = device.getQueue(indices.presentFamily.value(), 0);

    return queues;
}

std::vector<vk::Image> Renderer::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;

    // 0 means no limit to image count
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        imageCount = swapChainSupport.capabilities.maxImageCount;

    vk::SwapchainCreateInfoKHR createInfo{};
    createInfo.sType = vk::StructureType::eSwapchainCreateInfoKHR;
    createInfo.surface = surface;

    // the details and capabilities we selected
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = swapChainImageFormat;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = swapChainExtent;
    createInfo.presentMode = presentMode;

    // other settings
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque; // ignore alpha
    createInfo.clipped = vk::True;                                      // ingore hidden pixels (behind other windows for ex)
    createInfo.oldSwapchain = nullptr;

    // image sharing if multiple queues
    QueueFamilyIndices indices = Renderer::findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    swapChain = device.createSwapchainKHR(createInfo);
    check(swapChain);
    std::vector<vk::Image> swapChainImages = device.getSwapchainImagesKHR(swapChain);

    DIRK_LOG(LogVulkan, INFO,
             "created swap chain: "
                 << "\n\timage count: " << swapChainImages.size()
                 << "\n\timage width: " << swapChainExtent.width
                 << "\n\timage height: " << swapChainExtent.height);

    return swapChainImages;
};

vk::SurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

vk::PresentModeKHR Renderer::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        }
    }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Renderer::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;

    int width, height;

    // TODO: move to viewport
    // glfwGetFramebufferSize(window, &width, &height);

    vk::Extent2D actualExent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

    actualExent.width = std::clamp(actualExent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExent.height = std::clamp(actualExent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExent;
}

void Renderer::recreateSwapChain() {
    /**
    // wait if window has been minimized
    int width = 0, height = 0;
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    device.waitIdle();

    // recreate depth image
    this->depthImageMemoryView = createDepthResources();

    // recreate msaa image buffer
    this->colorImageMemoryView = createColorResources();

    // cleanup swap chain
    this->swapChainImages.clear();

    for (auto image : swapChainImages) {
        device.destroyFramebuffer(image.frameBuffer);
        device.destroyImageView(image.imageView);
    }

    this->device.destroySwapchainKHR(this->swapChain);

    std::vector<vk::Image> swapChainImages = createSwapChain();
    this->swapChainImages = createSwapChainImages(swapChainImages);

    Camera::get()->resize(swapChainExtent.width, swapChainExtent.height);
    */
}

vk::RenderPass Renderer::createRenderPass() {
    vk::AttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = msaaSamples;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::AttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::AttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = msaaSamples;
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
    colorAttachmentResolve.format = swapChainImageFormat;
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

    return device.createRenderPass(renderPassInfo);
}

vk::CommandPool Renderer::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = Renderer::findQueueFamilies(physicalDevice);

    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    return device.createCommandPool(poolInfo);
}

vk::DescriptorSetLayout Renderer::createDescriptorSetLayout() {
    std::array bindings = {
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr),
        vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr),
    };

    vk::DescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();
    return device.createDescriptorSetLayout(layoutInfo);
}

vk::DescriptorPool Renderer::createDescriptorPool() {
    std::array poolSizes{
        vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, MAX_DESCRIPTOR_SET_COUNT),
        vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, MAX_DESCRIPTOR_SET_COUNT),
    };
    vk::DescriptorPoolCreateInfo poolInfo{};
    poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    poolInfo.maxSets = MAX_DESCRIPTOR_SET_COUNT;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();

    return device.createDescriptorPool(poolInfo);
}

vk::Pipeline Renderer::createGraphicsPipeline() {
    vk::ShaderModule vert = Renderer::loadShaderModule("shader.vert");
    vk::ShaderModule frag = Renderer::loadShaderModule("shader.frag");

    // vert shader
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = vert;
    vertShaderStageInfo.pName = "main";

    // frag shader
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = frag;
    fragShaderStageInfo.pName = "main";

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
    multisampling.rasterizationSamples = msaaSamples;

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
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);
    if (!pipelineLayout)
        return nullptr;

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

    vk::Pipeline pipeline = device.createGraphicsPipeline(VK_NULL_HANDLE, pipelineInfo).value;

    device.destroyShaderModule(vert);
    device.destroyShaderModule(frag);

    return pipeline;
}

ImageMemoryView Renderer::createDepthResources() {
    this->depthFormat = Renderer::findSupportedFormat(
        physicalDevice,
        { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment);

    CreateImageMemoryViewInfo createInfo{
        .width = swapChainExtent.width,
        .height = swapChainExtent.height,
        .format = depthFormat,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
        .properties = vk::MemoryPropertyFlagBits::eDeviceLocal,
        .imageAspect = vk::ImageAspectFlagBits::eDepth,
        .numSamples = msaaSamples,
    };
    ImageMemoryView imageMemoryView = Renderer::createImageMemoryView(createInfo);

    vk::CommandBuffer commandBuffer = Renderer::beginSingleTimeCommands();
    Renderer::transitionImageLayout(commandBuffer, imageMemoryView.image, depthFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal, 1);
    Renderer::endSingleTimeCommands(commandBuffer, queues.graphicsQueue);

    return imageMemoryView;
}

ImageMemoryView Renderer::createColorResources() {
    CreateImageMemoryViewInfo createInfo{
        .width = swapChainExtent.width,
        .height = swapChainExtent.height,
        .format = swapChainImageFormat,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
        .properties = vk::MemoryPropertyFlagBits::eDeviceLocal,
        .numSamples = msaaSamples,
    };
    return Renderer::createImageMemoryView(createInfo);
}

std::vector<SwapChainImage> Renderer::createSwapChainImages(std::vector<vk::Image>& images) {
    std::vector<SwapChainImage> swapImages(images.size());
    semaphores.resize(images.size());

    for (int i = 0; i < images.size(); i++) {
        SwapChainImage image;

        // image view
        image.imageView = Renderer::createImageView(images[i], swapChainImageFormat);

        // frame buffers
        std::array attachments = { colorImageMemoryView.view, depthImageMemoryView.view, image.imageView };
        vk::FramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = vk::StructureType::eFramebufferCreateInfo;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = attachments.size();
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;
        image.frameBuffer = device.createFramebuffer(framebufferInfo);

        swapImages[i] = image;

        vk::SemaphoreCreateInfo semaphoreInfo{};
        semaphores[i] = std::tuple(device.createSemaphore(semaphoreInfo), device.createSemaphore(semaphoreInfo));
    }

    return swapImages;
}

std::vector<InFlightImage> Renderer::createInFlightImages(const int imageCount) {
    std::vector<InFlightImage> images(imageCount);

    // mass command buffer allocation
    vk::CommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
    cmdAllocInfo.commandPool = commandPool;
    cmdAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    cmdAllocInfo.commandBufferCount = imageCount;
    std::vector<vk::CommandBuffer> commandBuffers = device.allocateCommandBuffers(cmdAllocInfo);

    for (int i = 0; i < imageCount; i++) {
        InFlightImage image;

        // command buffers
        image.commandBuffer = commandBuffers[i];

        // fence for syncing
        vk::FenceCreateInfo fenceInfo{};
        fenceInfo.sType = vk::StructureType::eFenceCreateInfo;
        fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled; // create the fence as signaled to avoid stalling at first draw call

        image.inFlightFence = device.createFence(fenceInfo);

        check(image);
        images[i] = image;
    }

    return images;
}

vk::DescriptorSet Renderer::createDescriptorSets(vk::Buffer uniformBuffer, vk::Sampler sampler, vk::ImageView imageView, vk::ImageLayout layout) {
    vk::DescriptorSetAllocateInfo descAllocInfo{};
    descAllocInfo.descriptorPool = descriptorPool;
    descAllocInfo.descriptorSetCount = 1;
    descAllocInfo.pSetLayouts = &descriptorSetLayout;
    vk::DescriptorSet descriptorSet = device.allocateDescriptorSets(descAllocInfo)[0];

    // writes to the descriptor sets
    std::array descriptorWrites{
        vk::WriteDescriptorSet{},
        vk::WriteDescriptorSet{},
    };

    // set descriptor for ubo
    vk::DescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(ModelViewProjection);

    descriptorWrites[0].dstSet = descriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    vk::DescriptorImageInfo imageInfo{};
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;
    imageInfo.imageLayout = layout;

    descriptorWrites[1].dstSet = descriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
    descriptorWrites[1].pImageInfo = &imageInfo;

    device.updateDescriptorSets(descriptorWrites, {});
    return descriptorSet;
}

#ifdef ENABLE_VALIDATION_LAYERS
vk::Bool32 Renderer::debugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT messageType,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    LogLevel level;

    switch (messageSeverity) {
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
        level = ERROR;
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
        level = WARNING;
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
        level = INFO;
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        level = DEBUG;
        break;
    }

    DIRK_LOG(LogVulkanValidation, level, pCallbackData->pMessage);

    return vk::False;
}

bool Renderer::checkValidationLayerSupport() {
    auto availableLayers = vk::enumerateInstanceLayerProperties();

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
            if (strcmp(layerName, layerProperties.layerName) == 0)
                layerFound = true;

        if (!layerFound) {
            DIRK_LOG(LogVulkan, ERROR, "validation layer \"" << layerName << "\" not found");
            return false;
        }
    }

    return true;
}

vk::DebugUtilsMessengerEXT Renderer::setupDebugMessenger() {
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{};
    debugUtilsMessengerCreateInfoEXT.messageSeverity = severityFlags;
    debugUtilsMessengerCreateInfoEXT.messageType = messageTypeFlags;
    debugUtilsMessengerCreateInfoEXT.pfnUserCallback = &debugCallback;

    vk::detail::DispatchLoaderDynamic dispatcher(instance, vkGetInstanceProcAddr);
    return instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT, nullptr, dispatcher);
}
#endif

void Renderer::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex) {
    // TODO: record command buffer for scene in viewport
    /**
    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
    beginInfo.pInheritanceInfo = nullptr;

    checkVulkan(commandBuffer.begin(&beginInfo));

    vk::RenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = vk::StructureType::eRenderPassBeginInfo;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainImages[imageIndex].frameBuffer;

    // make sure to render on the entire screen
    renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
    renderPassInfo.renderArea.extent = swapChainExtent;

    // clear color is black with 100% opacity
    std::array<vk::ClearValue, 2> clearValues = { vk::ClearColorValue(0.f, 0.f, 0.f, 1.f), vk::ClearDepthStencilValue(1.f, 0.f) };
    renderPassInfo.clearValueCount = clearValues.size();
    renderPassInfo.pClearValues = clearValues.data();

    commandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

    // viewport is dynamic
    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    commandBuffer.setViewport(0, 1, &viewport);

    // scissor is dynamic
    vk::Rect2D scissor{};
    scissor.offset = vk::Offset2D(0, 0);
    scissor.extent = swapChainExtent;
    commandBuffer.setScissor(0, 1, &scissor);

    // for all actors: recordCommandBuffer
    for (auto pair : World::get()->getActors()) {
        pair.second->recordCommandBuffer(commandBuffer, pipelineLayout);
    }

    commandBuffer.endRenderPass();

    commandBuffer.end();
    */
}

ImageMemoryView Renderer::createImageMemoryView(CreateImageMemoryViewInfo& createInfo) {
    auto [image, memory] = createImage(
        createInfo.width, createInfo.height,
        createInfo.format,
        createInfo.tiling,
        createInfo.usage,
        createInfo.properties,
        createInfo.numSamples,
        createInfo.mipLevels);

    auto view = createImageView(
        image,
        createInfo.format,
        createInfo.imageAspect,
        createInfo.mipLevels);

    return ImageMemoryView{
        .image = image,
        .memory = memory,
        .view = view,
    };
}

std::tuple<vk::Image, vk::DeviceMemory> Renderer::createImage(
    uint32_t width, uint32_t height, vk::Format format,
    vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties,
    vk::SampleCountFlagBits numSamples, uint32_t mipLevels) {

    vk::ImageCreateInfo imageInfo{};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.format = format;
    imageInfo.extent = vk::Extent3D(width, height, 1);
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = numSamples;
    imageInfo.tiling = tiling;
    imageInfo.usage = usage;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;

    vk::Image image = device.createImage(imageInfo);

    vk::MemoryRequirements memRequirements = device.getImageMemoryRequirements(image);
    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    vk::DeviceMemory imageMemory = device.allocateMemory(allocInfo);
    device.bindImageMemory(image, imageMemory, 0);

    return std::tuple(image, imageMemory);
}

vk::ImageView Renderer::createImageView(vk::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels) {
    vk::ImageViewCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eImageViewCreateInfo;
    createInfo.image = image;
    createInfo.viewType = vk::ImageViewType::e2D;
    createInfo.format = format;
    // basic single layer image
    createInfo.subresourceRange = { aspectFlags, 0, 1, 0, 1 };
    createInfo.subresourceRange.levelCount = mipLevels;

    // dont touch color channels
    createInfo.components.r = vk::ComponentSwizzle::eIdentity;
    createInfo.components.g = vk::ComponentSwizzle::eIdentity;
    createInfo.components.b = vk::ComponentSwizzle::eIdentity;
    createInfo.components.a = vk::ComponentSwizzle::eIdentity;

    return device.createImageView(createInfo);
}

std::tuple<vk::Buffer, vk::DeviceMemory> Renderer::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) {
    // buffer
    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.sType = vk::StructureType::eBufferCreateInfo;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    vk::Buffer buffer = device.createBuffer(bufferInfo);

    // buffer memory
    vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(buffer);

    vk::MemoryAllocateInfo memoryAllocateInfo{};
    memoryAllocateInfo.sType = vk::StructureType::eMemoryAllocateInfo;
    memoryAllocateInfo.allocationSize = memRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    vk::DeviceMemory bufferMemory = device.allocateMemory(memoryAllocateInfo);

    device.bindBufferMemory(buffer, bufferMemory, 0);

    return std::tuple(buffer, bufferMemory);
}

uint32_t Renderer::findMemoryType(vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    DIRK_LOG(LogVulkan, FATAL, "failed to find suitable memory type");
    return -1;
}

vk::Format Renderer::findSupportedFormat(vk::PhysicalDevice physicalDevice, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
    for (const auto format : candidates) {
        vk::FormatProperties props = physicalDevice.getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    DIRK_LOG(LogVulkan, FATAL, "failed to find supported format");
    return vk::Format::eR32G32B32A32Sfloat; // random format
}

vk::SampleCountFlagBits Renderer::getMaxUsableSampleCount(vk::PhysicalDevice physicalDevice) {
    vk::PhysicalDeviceProperties physicalDeviceProperties = physicalDevice.getProperties();

    vk::SampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;

    if (counts & vk::SampleCountFlagBits::e64) return vk::SampleCountFlagBits::e64;
    if (counts & vk::SampleCountFlagBits::e32) return vk::SampleCountFlagBits::e32;
    if (counts & vk::SampleCountFlagBits::e16) return vk::SampleCountFlagBits::e16;
    if (counts & vk::SampleCountFlagBits::e8) return vk::SampleCountFlagBits::e8;
    if (counts & vk::SampleCountFlagBits::e4) return vk::SampleCountFlagBits::e4;
    if (counts & vk::SampleCountFlagBits::e2) return vk::SampleCountFlagBits::e2;

    return vk::SampleCountFlagBits::e1;
}

vk::CommandBuffer Renderer::beginSingleTimeCommands() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    vk::CommandPool commandPool = device.createCommandPool(poolInfo);

    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.commandPool = commandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = 1;

    vk::CommandBuffer commandBuffer = device.allocateCommandBuffers(allocInfo).front();

    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    commandBuffer.begin(beginInfo);

    return commandBuffer;
}

void Renderer::endSingleTimeCommands(vk::CommandBuffer& commandBuffer, vk::Queue queue) {
    commandBuffer.end();

    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    queue.submit(submitInfo);
    queue.waitIdle(); // TODO: use a fence for more optimized simultaneous ops
}

void Renderer::transitionImageLayout(vk::CommandBuffer commandBuffer, const vk::Image& image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels) {
    vk::ImageMemoryBarrier barrier{};
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.image = image;
    barrier.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
    barrier.subresourceRange.levelCount = mipLevels;

    if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;

        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
        }
    }

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    } else {
        DIRK_LOG(LogVulkan, FATAL, "unsupported layout transition");
        return;
    }

    commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, nullptr, barrier);
}

void Renderer::copyBufferToImage(vk::CommandBuffer commandBuffer, vk::Buffer& buffer, vk::Image& image, uint32_t width, uint32_t height) {
    vk::BufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource = { vk::ImageAspectFlagBits::eColor, 0, 0, 1 };
    region.imageOffset = 0;
    region.imageExtent = vk::Extent3D(width, height, 1);

    commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, { region });
}

void Renderer::generateMipmaps(vk::CommandBuffer commandBuffer, vk::Image& image, vk::Format imageFormat, uint32_t texWidth, uint32_t texHeight, uint32_t mipLevels) {
    vk::FormatProperties formatPropertes = physicalDevice.getFormatProperties(imageFormat);
    if (!(formatPropertes.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        DIRK_LOG(LogVulkan, FATAL, "texture image format does not support linear blitting");
        return;
    }

    vk::ImageMemoryBarrier barrier{};
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    uint32_t mipWidth = texWidth;
    uint32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) {
        // transfer to src optimal layout
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;

        barrier.subresourceRange.baseMipLevel = i - 1; // i - 1 is the bigger image
        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, nullptr, barrier);

        // blit the image
        vk::ArrayWrapper1D<vk::Offset3D, 2> srcOffsets, dstOffsets;
        srcOffsets[0] = vk::Offset3D(0, 0, 0);
        srcOffsets[1] = vk::Offset3D(mipWidth, mipHeight, 1);
        dstOffsets[0] = vk::Offset3D(0, 0, 0);
        dstOffsets[1] = vk::Offset3D(mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1);

        vk::ImageBlit blit{};
        blit.srcSubresource = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, i - 1, 0, 1);
        blit.srcOffsets = srcOffsets;
        blit.dstSubresource = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, i, 0, 1);
        blit.dstOffsets = dstOffsets;
        commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, { blit }, vk::Filter::eLinear);

        // transfer the image to shader read optimal layout
        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, nullptr, barrier);

        if (mipWidth > 1)
            mipWidth /= 2;
        if (mipHeight > 1)
            mipHeight /= 2;
    }

    // transfer last mip as it is not blitted from
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, nullptr, barrier);
}

bool Renderer::hasStencilComponent(vk::Format format) {
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

RendererFeatures Renderer::getRendererFeatures(vk::PhysicalDevice physicalDevice) {
    vk::PhysicalDeviceFeatures deviceFeatures = physicalDevice.getFeatures();
    return RendererFeatures{
        .anisotropy = deviceFeatures.samplerAnisotropy == vk::True,
        .msaaSamples = static_cast<int>(Renderer::getMaxUsableSampleCount(physicalDevice)),
    };
}

vk::ShaderModule Renderer::loadShaderModule(const std::string& shaderName) {
    std::shared_ptr<const Shader> shader = ResourceManager::loadShader(shaderName);
    check(shader);

    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.codeSize = shader->size;
    createInfo.pCode = reinterpret_cast<const uint32_t*>(shader->shader.data());

    return device.createShaderModule(createInfo);
};

QueueFamilyIndices Renderer::findQueueFamilies(vk::PhysicalDevice device) {
    QueueFamilyIndices indices;

    std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        // graphics queue
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
            indices.graphicsFamily = i;

        // present queue
        vk::Bool32 presentSupport = device.getSurfaceSupportKHR(i, surface);

        if (presentSupport)
            indices.presentFamily = i;

        // dont loop over every possible queue if we have the required ones already
        if (indices.isComplete())
            break;

        i++;
    }

    return indices;
}

} // namespace dirk
