#include "vulkan.hpp"
#include "core/asserts.hpp"
#include "core/globals.hpp"
#include "engine/dirkengine.hpp"
#include "render/render_types.hpp"
#include "render/renderer_types.hpp"
#include "vulkan_types.hpp"
#include "vulkan_utils.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "stb_image/stb_image.h"
#include "tiny_obj_loader/tiny_obj_loader.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

DEFINE_LOG_CATEGORY(LogVulkan)
DEFINE_LOG_CATEGORY(LogVulkanValidation)

namespace dirk {

VulkanRenderer::VulkanRenderer(RendererCreateInfo& createInfo) : rendererCreateInfo(createInfo) {
    check(rendererCreateInfo.api == VulkanApi);
}

int VulkanRenderer::init() {
    if (glfwInit() == GLFW_FALSE) {
        DIRK_LOG(LogVulkan, FATAL, "unable to initialize GLFW")
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    this->window = glfwCreateWindow(
        rendererCreateInfo.windowWdith,
        rendererCreateInfo.windowHeight,
        rendererCreateInfo.applicationName.c_str(), nullptr, nullptr);
    if (!this->window) {
        DIRK_LOG(LogVulkan, FATAL, "error creating GLFW window")
        return EXIT_FAILURE;
    }

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, VulkanRenderer::frameBufferResizeCallback);
    DIRK_LOG(LogVulkan, DEBUG, "successfully setup GLFW window")

    DIRK_LOG(LogVulkan, INFO, "initlializing Vulkan...");
    // TODO: surround with try/catch as in vulkan tutorial
    this->instance = createVulkanInstance();
    if (!this->instance) {
        DIRK_LOG(LogVulkan, FATAL, "instance creation failed")
        return EXIT_FAILURE;
    }
#ifdef ENABLE_VALIDATION_LAYERS
    this->debugMessenger = setupDebugMessenger();
    if (!this->debugMessenger)
        DIRK_LOG(LogVulkan, ERROR, "failed to create vulkan layer validation debug messenger")
#endif
    this->surface = createSurface();
    if (!this->surface) {
        DIRK_LOG(LogVulkan, FATAL, "surface creation failed")
        return EXIT_FAILURE;
    }

    this->physicalDevice = getPhysicalDevice();
    if (!this->surface) {
        DIRK_LOG(LogVulkan, FATAL, "failed to get a physical device")
        return EXIT_FAILURE;
    }

    this->device = createLogicalDevice();
    if (!this->device) {
        DIRK_LOG(LogVulkan, FATAL, "failed to create logical device");
        return EXIT_FAILURE;
    }

    this->queues = createQueues();

    std::vector<vk::Image> swapChainImages = createSwapChain(VK_NULL_HANDLE);

    this->commandPool = createCommandPool();
    if (!this->commandPool) {
        DIRK_LOG(LogVulkan, FATAL, "failed to create command pool");
        return EXIT_FAILURE;
    }

    this->descriptorSetLayout = createDescriptorSetLayout();
    if (!this->descriptorSetLayout) {
        DIRK_LOG(LogVulkan, FATAL, "failed to create descriptor set layout");
        return EXIT_FAILURE;
    }

    this->colorImageMemoryView = createColorResources();
    if (!this->colorImageMemoryView) {
        DIRK_LOG(LogVulkan, FATAL, "failed to create color image");
        return EXIT_FAILURE;
    }

    this->depthImageMemoryView = createDepthResources();
    if (!this->depthImageMemoryView) {
        DIRK_LOG(LogVulkan, FATAL, "failed to create depth image");
        return EXIT_FAILURE;
    }

    this->renderPass = createRenderPass();
    if (!this->renderPass) {
        DIRK_LOG(LogVulkan, FATAL, "failed to create render pass");
        return EXIT_FAILURE;
    }

    this->graphicsPipeline = createGraphicsPipeline();
    if (!this->graphicsPipeline) {
        DIRK_LOG(LogVulkan, FATAL, "failed to create graphics pipeline");
        return EXIT_FAILURE;
    }

    this->descriptorPool = createDescriptorPool();
    if (!this->descriptorPool) {
        DIRK_LOG(LogVulkan, FATAL, "failed to create descriptor pool");
        return EXIT_FAILURE;
    }

    if (!loadModel()) {
        DIRK_LOG(LogVulkan, FATAL, "failed to load model");
        return EXIT_FAILURE;
    }

    this->textureImageMemoryView = createTextureResources();
    if (!this->textureImageMemoryView) {
        DIRK_LOG(LogVulkan, FATAL, "failed to create texture image");
        return EXIT_FAILURE;
    }

    this->textureSampler = createTextureSampler();
    if (!this->textureSampler) {
        DIRK_LOG(LogVulkan, FATAL, "failed to create texture sampler");
        return EXIT_FAILURE;
    }

    this->vertexBuffer = createVertexBuffer();
    if (!this->vertexBuffer) {
        DIRK_LOG(LogVulkan, FATAL, "failed to create vertex buffer");
        return EXIT_FAILURE;
    }

    this->indexBuffer = createIndexBuffer();
    if (!this->indexBuffer) {
        DIRK_LOG(LogVulkan, FATAL, "failed to create index buffer");
        return EXIT_FAILURE;
    }

    this->swapChainImages = createSwapChainImages(swapChainImages);
    this->inFlightImages = createInFlightImages(MAX_FRAMES_IN_FLIGHT);

    DIRK_LOG(LogVulkan, INFO, "vulkan initialized successfully");
    return EXIT_SUCCESS;
}

void VulkanRenderer::draw(float deltaTime) {
    if (glfwWindowShouldClose(window)) {
        dirk::gEngine->exit("GLFW close event");
    }

    updateMVP(deltaTime);
    drawFrame();
}

void VulkanRenderer::cleanup() {
    // make sure all device ops are finished
    device.waitIdle();
    DIRK_LOG(LogVulkan, INFO, "cleaning up renderer");

    glfwDestroyWindow(window);
    glfwTerminate();
}

vk::Instance VulkanRenderer::createVulkanInstance() {
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

    try {
        return vk::createInstance(createInfo);
    } catch (const std::exception& err) {
        DIRK_LOG(LogVulkan, FATAL, err.what());
        return nullptr;
    }
}

std::vector<const char*> VulkanRenderer::getRequiredInstanceExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#ifdef ENABLE_VALIDATION_LAYERS
    extensions.push_back(vk::EXTDebugUtilsExtensionName);
#endif

    return extensions;
}

bool VulkanRenderer::checkRequiredInstanceExtensions(std::vector<const char*>& extensions) {
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

vk::SurfaceKHR VulkanRenderer::createSurface() {
    VkSurfaceKHR surfaceTmp;
    glfwCreateWindowSurface(instance, window, nullptr, &surfaceTmp);
    return vk::SurfaceKHR(surfaceTmp);
}

vk::PhysicalDevice VulkanRenderer::getPhysicalDevice() {
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
        msaaSamples = VulkanUtils::getMaxUsableSampleCount(physicalDevice);
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

    this->features = VulkanUtils::getRendererFeatures(physicalDevice);

    return physicalDevice;
}

int VulkanRenderer::getDeviceSuitability(vk::PhysicalDevice device) {
    check(device);

    vk::PhysicalDeviceProperties deviceProperties = device.getProperties();
    vk::PhysicalDeviceFeatures deviceFeatures = device.getFeatures();

    // TODO: update with vulkan tutorial checks

    DIRK_LOG(LogVulkan, DEBUG, "found device: " << deviceProperties.deviceName);

    // prereturn required stuff
    if (!deviceFeatures.geometryShader)
        return 0;

    QueueFamilyIndices indices = findQueueFamilies(device);
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

    score += VulkanUtils::getRendererFeatures(device).getScore();

    return score;
}

QueueFamilyIndices VulkanRenderer::findQueueFamilies(vk::PhysicalDevice device) {
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

bool VulkanRenderer::checkDeviceExtensionSupport(vk::PhysicalDevice device) {
    std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties();

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

SwapChainSupportDetails VulkanRenderer::querySwapChainSupport(vk::PhysicalDevice device) {
    return SwapChainSupportDetails{
        .capabilities = device.getSurfaceCapabilitiesKHR(surface),
        .formats = device.getSurfaceFormatsKHR(surface),
        .presentModes = device.getSurfacePresentModesKHR(surface),
    };
}

vk::Device VulkanRenderer::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

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

Queues VulkanRenderer::createQueues() {
    Queues queues;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    queues.graphicsQueue = device.getQueue(indices.graphicsFamily.value(), 0);
    queues.presentQueue = device.getQueue(indices.presentFamily.value(), 0);

    return queues;
}

std::vector<vk::Image> VulkanRenderer::createSwapChain(vk::SwapchainKHR oldSwapChain) {
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
    createInfo.oldSwapchain = oldSwapChain;

    // image sharing if multiple queues
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
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

vk::SurfaceFormatKHR VulkanRenderer::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

vk::PresentModeKHR VulkanRenderer::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        }
    }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D VulkanRenderer::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;

    int width, height;

    glfwGetFramebufferSize(window, &width, &height);

    vk::Extent2D actualExent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

    actualExent.width = std::clamp(actualExent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExent.height = std::clamp(actualExent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExent;
}

void VulkanRenderer::recreateSwapChain() {
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
    this->swapChain = nullptr;

    std::vector<vk::Image> swapChainImages = createSwapChain(this->swapChain);
    this->swapChainImages = createSwapChainImages(swapChainImages);
}

vk::RenderPass VulkanRenderer::createRenderPass() {
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

vk::CommandPool VulkanRenderer::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    return device.createCommandPool(poolInfo);
}

vk::DescriptorSetLayout VulkanRenderer::createDescriptorSetLayout() {
    std::array bindings = {
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr),
        vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr),
    };

    vk::DescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();
    return device.createDescriptorSetLayout(layoutInfo);
}

vk::DescriptorPool VulkanRenderer::createDescriptorPool() {
    std::array poolSizes{
        vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT),
        vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT),
    };
    vk::DescriptorPoolCreateInfo poolInfo{};
    poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();

    return device.createDescriptorPool(poolInfo);
}

vk::Pipeline VulkanRenderer::createGraphicsPipeline() {
    vk::ShaderModule vert = loadShaderModule("shader.vert");
    vk::ShaderModule frag = loadShaderModule("shader.frag");

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

    auto bindingDescription = VulkanVertex::getBindingDescription();
    auto attributeDescriptons = VulkanVertex::getAttributeDescriptions();
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

ImageMemoryView VulkanRenderer::createDepthResources() {
    this->depthFormat = VulkanUtils::findSupportedFormat(
        physicalDevice,
        { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment);

    CreateImageMemoryViewInfo createInfo{
        .device = device,
        .physicalDevice = physicalDevice,
        .width = swapChainExtent.width,
        .height = swapChainExtent.height,
        .format = depthFormat,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
        .properties = vk::MemoryPropertyFlagBits::eDeviceLocal,
        .imageAspect = vk::ImageAspectFlagBits::eDepth,
        .numSamples = msaaSamples,
    };
    ImageMemoryView imageMemoryView = VulkanUtils::createImageMemoryView(createInfo);

    vk::CommandBuffer commandBuffer = VulkanUtils::beginSingleTimeCommands(device, commandPool);
    VulkanUtils::transitionImageLayout(commandBuffer, imageMemoryView.image, depthFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal, 1);
    VulkanUtils::endSingleTimeCommands(commandBuffer, queues.graphicsQueue);

    return imageMemoryView;
}

ImageMemoryView VulkanRenderer::createColorResources() {
    CreateImageMemoryViewInfo createInfo{
        .device = device,
        .physicalDevice = physicalDevice,
        .width = swapChainExtent.width,
        .height = swapChainExtent.height,
        .format = swapChainImageFormat,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
        .properties = vk::MemoryPropertyFlagBits::eDeviceLocal,
        .numSamples = msaaSamples,
    };
    return VulkanUtils::createImageMemoryView(createInfo);
}

ImageMemoryView VulkanRenderer::createTextureResources() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load((std::string(RESSOURCE_PATH) + "/textures/viking_room.png").c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    vk::DeviceSize imageSize = texWidth * texHeight * 4; // 4 bytes per pixel (1 per channel)

    this->mipLevels = std::floor(std::log2(std::max(texWidth, texHeight))) + 1;

    check(pixels);

    vk::Format format = vk::Format::eR8G8B8A8Srgb;

    auto [stagingBuffer, stagingBufferMemory] = VulkanUtils::createBuffer(
        device, physicalDevice, imageSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void* data = device.mapMemory(stagingBufferMemory, 0, imageSize);
    memcpy(data, pixels, imageSize);
    device.unmapMemory(stagingBufferMemory);

    stbi_image_free(pixels);

    CreateImageMemoryViewInfo createInfo{
        .device = device,
        .physicalDevice = physicalDevice,
        .width = static_cast<uint32_t>(texWidth),
        .height = static_cast<uint32_t>(texHeight),
        .format = format,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        .properties = vk::MemoryPropertyFlagBits::eDeviceLocal,
        .mipLevels = mipLevels,
    };
    ImageMemoryView imageMemoryView = VulkanUtils::createImageMemoryView(createInfo);

    vk::CommandBuffer commandBuffer = VulkanUtils::beginSingleTimeCommands(device, commandPool);
    VulkanUtils::transitionImageLayout(commandBuffer, imageMemoryView.image, format, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, mipLevels);
    VulkanUtils::copyBufferToImage(commandBuffer, stagingBuffer, imageMemoryView.image, texWidth, texHeight);
    VulkanUtils::generateMipmaps(commandBuffer, physicalDevice, imageMemoryView.image, format, texWidth, texHeight, mipLevels);
    // transitions to vk::ImageLayout::eShaderReadOnlyOptimal while generating mipmaps
    VulkanUtils::endSingleTimeCommands(commandBuffer, queues.graphicsQueue);

    return imageMemoryView;
}

vk::Buffer VulkanRenderer::createVertexBuffer() {
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    auto [stagingBuffer, stagingBufferMemory] = VulkanUtils::createBuffer(
        device, physicalDevice, bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void* dataStaging = device.mapMemory(stagingBufferMemory, 0, bufferSize);
    memcpy(dataStaging, vertices.data(), bufferSize);
    device.unmapMemory(stagingBufferMemory);

    auto [buffer, bufferMemory] = VulkanUtils::createBuffer(
        device, physicalDevice, bufferSize,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    vk::CommandBuffer commandBuffer = VulkanUtils::beginSingleTimeCommands(device, commandPool);
    commandBuffer.copyBuffer(stagingBuffer, buffer, vk::BufferCopy(0, 0, bufferSize));
    VulkanUtils::endSingleTimeCommands(commandBuffer, queues.graphicsQueue);

    return buffer;
}

vk::Buffer VulkanRenderer::createIndexBuffer() {
    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    auto [stagingBuffer, stagingBufferMemory] = VulkanUtils::createBuffer(
        device, physicalDevice, bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void* data = device.mapMemory(stagingBufferMemory, 0, bufferSize);
    memcpy(data, indices.data(), bufferSize);
    device.unmapMemory(stagingBufferMemory);

    auto [buffer, bufferMemory] = VulkanUtils::createBuffer(
        device, physicalDevice, bufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    vk::CommandBuffer commandBuffer = VulkanUtils::beginSingleTimeCommands(device, commandPool);
    commandBuffer.copyBuffer(stagingBuffer, buffer, vk::BufferCopy(0, 0, bufferSize));
    VulkanUtils::endSingleTimeCommands(commandBuffer, queues.graphicsQueue);

    return buffer;
}

bool VulkanRenderer::loadModel() {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, (std::string(RESSOURCE_PATH) + "/models/viking_room.obj").c_str())) {
        if (warn != "") {
            warn.pop_back(); // remove trailing return
            DIRK_LOG(LogVulkan, WARNING, "tiny obj loader: " << warn);
        }
        if (err != "") {
            err.pop_back(); // remove trailing return
            DIRK_LOG(LogVulkan, ERROR, "tiny obj loader: " << err);
        }
        return false;
    }

    if (warn != "") {
        warn.pop_back(); // remove trailing return
        DIRK_LOG(LogVulkan, WARNING, "tiny obj loader: " << warn);
    }
    if (err != "") {
        err.pop_back(); // remove trailing return
        DIRK_LOG(LogVulkan, ERROR, "tiny obj loader: " << err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    int vertex_count = attrib.vertices.size() / 3;
    vertices.reserve(vertex_count);
    indices.reserve(vertex_count);
    uniqueVertices.reserve(vertex_count);

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.f - attrib.texcoords[2 * index.texcoord_index + 1], // flip the image as tinyobj assumes 0 is bottom; vulkan assumes 0 is top
            };

            vertex.color = { 1.f, 1.f, 1.f };

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = vertices.size();
                vertices.emplace_back(vertex);
            }

            indices.emplace_back(uniqueVertices[vertex]);
        }
    }

    return true;
}

vk::Sampler VulkanRenderer::createTextureSampler() {
    vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
    vk::SamplerCreateInfo samplerInfo{};

    samplerInfo.magFilter = vk::Filter::eLinear;
    samplerInfo.minFilter = vk::Filter::eLinear;

    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.mipLodBias = 0.f;
    samplerInfo.minLod = 0.f;
    samplerInfo.maxLod = vk::LodClampNone;

    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;

    samplerInfo.anisotropyEnable = getFeatures().anisotropy ? vk::True : vk::False;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

    samplerInfo.compareEnable = vk::False;
    samplerInfo.compareOp = vk::CompareOp::eAlways;
    samplerInfo.unnormalizedCoordinates = vk::False; // the tex coords are normalized

    return device.createSampler(samplerInfo);
}
std::vector<SwapChainImage> VulkanRenderer::createSwapChainImages(std::vector<vk::Image>& images) {
    std::vector<SwapChainImage> swapImages(images.size());
    semaphores.resize(images.size());

    for (int i = 0; i < images.size(); i++) {
        SwapChainImage image;

        // image view
        image.imageView = VulkanUtils::createImageView(device, images[i], swapChainImageFormat);

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

std::vector<InFlightImage> VulkanRenderer::createInFlightImages(const int imageCount) {
    std::vector<InFlightImage> images(imageCount);

    // mass command buffer allocation
    vk::CommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
    cmdAllocInfo.commandPool = commandPool;
    cmdAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    cmdAllocInfo.commandBufferCount = imageCount;
    std::vector<vk::CommandBuffer> commandBuffers = device.allocateCommandBuffers(cmdAllocInfo);

    // mass descriptor set layout allocation
    std::vector<vk::DescriptorSetLayout> layouts(imageCount, descriptorSetLayout);
    vk::DescriptorSetAllocateInfo descAllocInfo{};
    descAllocInfo.descriptorPool = descriptorPool;
    descAllocInfo.descriptorSetCount = layouts.size();
    descAllocInfo.pSetLayouts = layouts.data();
    std::vector<vk::DescriptorSet> descriptorSets = device.allocateDescriptorSets(descAllocInfo);

    for (int i = 0; i < imageCount; i++) {
        InFlightImage image;

        // command buffers
        image.commandBuffer = commandBuffers[i];

        // fence for syncing
        vk::FenceCreateInfo fenceInfo{};
        fenceInfo.sType = vk::StructureType::eFenceCreateInfo;
        fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled; // create the fence as signaled to avoid stalling at first draw call

        image.inFlightFence = device.createFence(fenceInfo);

        // ubo buffers for mvp
        vk::DeviceSize bufferSize = sizeof(ModelViewProjection);
        auto [uniformBuffer, uniformBufferMemory] = VulkanUtils::createBuffer(
            device, physicalDevice, bufferSize,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        image.uniformBuffer = uniformBuffer;
        image.uniformBufferMemory = uniformBufferMemory;
        image.uniformBufferMapped = device.mapMemory(uniformBufferMemory, 0, bufferSize);

        // descriptor sets

        // writes to the descriptor sets
        std::array descriptorWrites{
            vk::WriteDescriptorSet{},
            vk::WriteDescriptorSet{},
        };

        // set descriptor for ubo
        vk::DescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = image.uniformBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(ModelViewProjection);

        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        // set descriptor for texture sampling
        vk::DescriptorImageInfo imageInfo{};
        imageInfo.imageView = textureImageMemoryView.view;
        imageInfo.sampler = textureSampler;
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        descriptorWrites[1].pImageInfo = &imageInfo;

        device.updateDescriptorSets(descriptorWrites, {});
        image.descriptorSet = descriptorSets[i];

        check(image);
        images[i] = image;
    }

    return images;
}

#ifdef ENABLE_VALIDATION_LAYERS
vk::Bool32 VulkanRenderer::debugCallback(
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

bool VulkanRenderer::checkValidationLayerSupport() {
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

vk::DebugUtilsMessengerEXT VulkanRenderer::setupDebugMessenger() {
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

void VulkanRenderer::frameBufferResizeCallback(GLFWwindow* window, int width, int height) {
    // TODO: reinterpret_cast stops the program with no error?????????
    // VulkanRenderer* renderer = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
    // renderer->framebufferResized = true;
}

void VulkanRenderer::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex) {
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
    commandBuffer.bindVertexBuffers(0, vertexBuffer, { 0 });
    commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, inFlightImages[currentFrame].descriptorSet, nullptr);

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

    commandBuffer.drawIndexed(indices.size(), 1, 0, 0, 0);

    commandBuffer.endRenderPass();

    // TODO: how to error handling in command buffer
    commandBuffer.end();
}

void VulkanRenderer::drawFrame() {
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

void VulkanRenderer::updateMVP(float deltaTime) {
    ModelViewProjection mvp{
        .model = glm::rotate(glm::mat4(1.f), 0.f, glm::vec3(0.f, 0.f, 1.f)),
        .view = glm::lookAt(glm::vec3(2.f, 2.f, 2.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 1.f)),
        .proj = glm::perspective(glm::radians(45.f), static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height), .1f, 10.f),
    };

    mvp.proj[1][1] *= -1; // glm was originally designed for OpenGL. We must thus flip the y axis of the projection matrix

    memcpy(inFlightImages[currentFrame].uniformBufferMapped, &mvp, sizeof(mvp));
}

vk::ShaderModule VulkanRenderer::loadShaderModule(const std::string& shaderName) {
    std::ifstream file(std::string(SHADER_PATH) + "/" + shaderName + ".spv", std::ios::ate | std::ios::binary);

    check(file.is_open());

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> shader(fileSize);

    file.seekg(0);
    file.read(shader.data(), fileSize);

    file.close();

    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eShaderModuleCreateInfo;
    createInfo.codeSize = shader.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(shader.data());

    vk::ShaderModule module = device.createShaderModule(createInfo);
    check(module);
    return module;
}

} // namespace dirk
