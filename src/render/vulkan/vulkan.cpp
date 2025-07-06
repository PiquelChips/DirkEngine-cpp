#include "render/vulkan/vulkan.hpp"
#include "engine/dirkengine.hpp"
#include "logger.hpp"
#include "render/render.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <vector>

VulkanRenderer::VulkanRenderer(RendererConfig rendererConfig, Logger* logger) : rendererConfig(rendererConfig), logger(logger) {}

int VulkanRenderer::init() {
    int result = EXIT_SUCCESS;

    result = initWindow();
    if (result != EXIT_SUCCESS)
        return result;

    result = initVulkan();
    if (result != EXIT_SUCCESS)
        return result;

    getLogger()->Get(INFO) << "engine initialization successful";
    return result;
}

void VulkanRenderer::tick(DirkEngine* engine) {
    if (glfwWindowShouldClose(window))
        engine->exit("GLFW close event");
}

int VulkanRenderer::draw() {
    drawFrame();
    return EXIT_SUCCESS;
}

void VulkanRenderer::cleanup() {
    // make sure all device ops are finished
    device.waitIdle();
    getLogger()->Get(INFO) << "cleaning up renderer";

    glfwDestroyWindow(window);
    glfwTerminate();
}

int VulkanRenderer::initWindow() {
    if (glfwInit() == GLFW_FALSE) {
        getLogger()->Get(FATAL) << "unable to initialize GLFW";
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(rendererConfig.width, rendererConfig.height, rendererConfig.name.c_str(), nullptr, nullptr);
    if (window == nullptr) {
        getLogger()->Get(FATAL) << "error creating GLFW window";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int VulkanRenderer::initVulkan() {
    getLogger()->Get(INFO) << "Initlializing Vulkan...";

    // TODO: surround with try/catch as in vulkan tutorial

    createVulkanInstance();
#ifdef ENABLE_VALIDATION_LAYERS
    setupDebugMessenger();
#endif
    createSurface();
    getPhysicalDevice();
    createLogicalDevice();

    std::vector<vk::Image> swapChainImages = createSwapChain();
    createRenderPass();
    createCommandPool();
    createGraphicsPipeline();

    createSwapChainImages(swapChainImages);
    createInFlightImages(MAX_FRAMES_IN_FLIGHT);

    getLogger()->Get(INFO) << "vulkan initialized successfully";

    return EXIT_SUCCESS;
}

void VulkanRenderer::createVulkanInstance() {
    // vk app
    vk::ApplicationInfo appInfo{};
    appInfo.sType = vk::StructureType::eApplicationInfo;
    appInfo.pApplicationName = "DirkEngine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "DirkEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = vk::ApiVersion14;

    auto instanceExtensions = getRequiredInstanceExtensions();

    vk::InstanceCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eInstanceCreateInfo;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = instanceExtensions.size();
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();

#ifdef ENABLE_VALIDATION_LAYERS
    assert(checkValidationLayerSupport());
    getLogger()->Get(INFO) << "using validation layers";
    createInfo.enabledLayerCount = validationLayers.size();
    createInfo.ppEnabledLayerNames = validationLayers.data();
#else
    createInfo.enabledLayerCount = 0;
#endif

    instance = vk::createInstance(createInfo);

    getLogger()->Get(INFO) << "instance creation successful";
}

std::vector<const char*> VulkanRenderer::getRequiredInstanceExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#ifdef ENABLE_VALIDATION_LAYERS
    extensions.push_back(vk::EXTDebugUtilsExtensionName);
#endif

    // TODO: make sure all extensions are supported by the driver

    return extensions;
}

void VulkanRenderer::createSurface() {
    VkSurfaceKHR surfaceTmp;
    assert(glfwCreateWindowSurface(instance, window, nullptr, &surfaceTmp) == VK_SUCCESS);
    surface = vk::SurfaceKHR(surfaceTmp);
    getLogger()->Get(INFO) << "surface creation successful";
}

void VulkanRenderer::getPhysicalDevice() {
    auto devices = instance.enumeratePhysicalDevices();

    // rank each available device
    std::multimap<int, vk::PhysicalDevice> candidates;

    for (const auto& device : devices) {
        int score = getDeviceSuitability(device);
        candidates.insert(std::make_pair(score, device));
    }

    if (candidates.rbegin()->first > 0) {
        physicalDevice = candidates.rbegin()->second;
    } else {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    vk::PhysicalDeviceProperties deviceProperties = physicalDevice.getProperties();
    // TODO: get more human readable data (like enum values)
    getLogger()->Get(INFO)
        << "physical device selected: "
        << "\n\tvendor id: " << deviceProperties.vendorID
        << "\n\tdevice id: " << deviceProperties.deviceID
        << "\n\tdevice name: " << deviceProperties.deviceName
        //<< "\n\tdevice type: " << deviceProperties.deviceType
        << "\n\tapi version: " << deviceProperties.apiVersion
        << "\n\tdriver version: " << deviceProperties.driverVersion;
}

int VulkanRenderer::getDeviceSuitability(vk::PhysicalDevice device) {
    vk::PhysicalDeviceProperties deviceProperties = device.getProperties();
    vk::PhysicalDeviceFeatures deviceFeatures = device.getFeatures();

    // TODO: update with vulkan tutorial checks

    getLogger()->Get(INFO) << "found device: " << deviceProperties.deviceName;

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

void VulkanRenderer::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    // queues
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos(uniqueQueueFamilies.size());
    float queuePriority = 1.f;
    for (int i = 0; i < uniqueQueueFamilies.size(); i++) {
        std::set<uint32_t>::iterator iter = uniqueQueueFamilies.find(i);
        if (iter == uniqueQueueFamilies.end())
            return;

        uint32_t queueFamily = *iter;

        vk::DeviceQueueCreateInfo queueCreateInfo;
        queueCreateInfo.sType = vk::StructureType::eDeviceQueueCreateInfo;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfos[i] = queueCreateInfo;
    }

    vk::PhysicalDeviceFeatures deviceFeatures{};

    vk::DeviceCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eDeviceCreateInfo;
    createInfo.pEnabledFeatures = &deviceFeatures;
    // queues
    createInfo.queueCreateInfoCount = queueCreateInfos.size();
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    // extensions
    createInfo.enabledExtensionCount = deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    device = physicalDevice.createDevice(createInfo);

    getLogger()->Get(INFO) << "logical Vulkan device creation successful";

    device.getQueue(indices.graphicsFamily.value(), 0, &queues.graphicsQueue);
    device.getQueue(indices.presentFamily.value(), 0, &queues.presentQueue);
}

std::vector<vk::Image> VulkanRenderer::createSwapChain() {
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
    createInfo.oldSwapchain = VK_NULL_HANDLE;

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
    std::vector<vk::Image> swapChainImages = device.getSwapchainImagesKHR(swapChain);

    getLogger()->Get(INFO) << "created swap chain: "
                           << "\n\timage count: " << swapChainImages.size()
                           << "\n\timage width: " << swapChainExtent.width
                           << "\n\timage height: " << swapChainExtent.height;

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

void VulkanRenderer::createRenderPass() {
    vk::AttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass{};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    vk::SubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

    vk::RenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = vk::StructureType::eRenderPassCreateInfo;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    renderPass = device.createRenderPass(renderPassInfo);
}

void VulkanRenderer::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.sType = vk::StructureType::eCommandPoolCreateInfo,
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value(),

    commandPool = device.createCommandPool(poolInfo);
}

void VulkanRenderer::createGraphicsPipeline() {
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

    // vertex input
    // hardcoded in vert shader for now
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = vk::StructureType::ePipelineVertexInputStateCreateInfo;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;

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
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable = vk::False;

    // disabled for now
    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = vk::StructureType::ePipelineMultisampleStateCreateInfo;
    multisampling.sampleShadingEnable = vk::False;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

    // color blending
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = vk::False;

    vk::PipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = vk::StructureType::ePipelineColorBlendStateCreateInfo;
    colorBlending.logicOpEnable = vk::False;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

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
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;

    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    graphicsPipeline = device.createGraphicsPipeline(VK_NULL_HANDLE, pipelineInfo).value;

    device.destroyShaderModule(vert);
    device.destroyShaderModule(frag);
}

void VulkanRenderer::createSwapChainImages(std::vector<vk::Image> images) {
    swapChainImages.resize(images.size());

    for (int i = 0; i < images.size(); i++) {
        SwapChainImage image = swapChainImages[i];

        // image view

        vk::ImageViewCreateInfo createInfo{};
        createInfo.sType = vk::StructureType::eImageViewCreateInfo;
        createInfo.image = images[i];
        createInfo.viewType = vk::ImageViewType::e2D;
        createInfo.format = swapChainImageFormat;

        // dont touch color channels
        createInfo.components.r = vk::ComponentSwizzle::eIdentity;
        createInfo.components.g = vk::ComponentSwizzle::eIdentity;
        createInfo.components.b = vk::ComponentSwizzle::eIdentity;
        createInfo.components.a = vk::ComponentSwizzle::eIdentity;

        // basic single layer image
        createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        image.imageView = device.createImageView(createInfo);

        // frame buffers

        vk::FramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = vk::StructureType::eFramebufferCreateInfo;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &image.imageView;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        image.frameBuffer = device.createFramebuffer(framebufferInfo);
    }
}

void VulkanRenderer::createInFlightImages(const int imageCount) {
    inFlightImages.resize(imageCount);

    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
    allocInfo.commandPool = commandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = imageCount;

    std::vector<vk::CommandBuffer> commandBuffers = device.allocateCommandBuffers(allocInfo);

    for (int i = 0; i < imageCount; i++) {
        InFlightImage image = inFlightImages[i];

        // command buffers
        image.commandBuffer = commandBuffers[i];

        // sync objects
        vk::SemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = vk::StructureType::eSemaphoreCreateInfo;

        vk::FenceCreateInfo fenceInfo{};
        fenceInfo.sType = vk::StructureType::eFenceCreateInfo;
        fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled; // create the fence as signaled to avoid stalling at first draw call

        image.imageAvailableSemaphore = device.createSemaphore(semaphoreInfo);
        image.renderFinishedSemaphore = device.createSemaphore(semaphoreInfo);
        image.inFlightFence = device.createFence(fenceInfo);
    }
}

#ifdef ENABLE_VALIDATION_LAYERS
vk::Bool32 VulkanRenderer::debugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT messageType,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    VulkanRenderer* renderer = static_cast<VulkanRenderer*>(pUserData);
    assert(renderer);

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

    renderer->getLogger()->Get(level) << "[Vulkan] " << pCallbackData->pMessage;

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
            getLogger()->Get(ERROR) << "validation layer \"" << layerName << "\" not found";
            return false;
        }
    }

    return true;
}

void VulkanRenderer::setupDebugMessenger() {
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{};
    debugUtilsMessengerCreateInfoEXT.messageSeverity = severityFlags;
    debugUtilsMessengerCreateInfoEXT.messageType = messageTypeFlags;
    debugUtilsMessengerCreateInfoEXT.pfnUserCallback = &debugCallback;
    debugUtilsMessengerCreateInfoEXT.pUserData = this;

    vk::detail::DispatchLoaderDynamic dispatcher(instance, vkGetInstanceProcAddr);
    debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT, nullptr, dispatcher);
    assert(debugMessenger);
    getLogger()->Get(INFO) << "debug messenger created";
}
#endif

void VulkanRenderer::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex) {
    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
    beginInfo.pInheritanceInfo = nullptr;

    assert(commandBuffer.begin(&beginInfo) == vk::Result::eSuccess);

    vk::RenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = vk::StructureType::eRenderPassBeginInfo;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainImages[imageIndex].frameBuffer;

    // make sure to render on the entire screen
    renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
    renderPassInfo.renderArea.extent = swapChainExtent;

    // clear color is black with 100% opacity
    vk::ClearValue clearColor{ { 0.0f, 0.0f, 0.0f, 1.0f } };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

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

    commandBuffer.draw(3, 1, 0, 0);

    commandBuffer.endRenderPass();

    // TODO: how to error handling in command buffer
    commandBuffer.end();
}

void VulkanRenderer::drawFrame() {
    InFlightImage image = inFlightImages[currentFrame];

    // wait for previous frame
    assert(device.waitForFences(1, &image.inFlightFence, vk::True, UINT64_MAX) == vk::Result::eSuccess);
    assert(device.resetFences(1, &image.inFlightFence) == vk::Result::eSuccess);

    // acquire image from swapChain
    uint32_t imageIndex;
    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, image.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    // record the command buffer
    vkResetCommandBuffer(image.commandBuffer, 0);
    recordCommandBuffer(image.commandBuffer, imageIndex);

    // submit the command buffer
    vk::SubmitInfo submitInfo{};
    submitInfo.sType = vk::StructureType::eSubmitInfo;

    // wait semaphores
    vk::Semaphore waitSemaphores[] = { image.imageAvailableSemaphore }; // index has to match with stage in `waitStages`
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    // signal semaphores
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &image.renderFinishedSemaphore;

    // command buffers
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &image.commandBuffer;

    assert(queues.graphicsQueue.submit(1, &submitInfo, image.inFlightFence) == vk::Result::eSuccess);

    vk::PresentInfoKHR presentInfo{};
    presentInfo.sType = vk::StructureType::ePresentInfoKHR;

    // make sure to wait for the image to be rendered
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &image.renderFinishedSemaphore;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // only have one swap chain

    assert(queues.presentQueue.presentKHR(&presentInfo) == vk::Result::eSuccess);

    currentFrame = (++currentFrame) % MAX_FRAMES_IN_FLIGHT;
}

vk::ShaderModule VulkanRenderer::loadShaderModule(const std::string& shaderName) {
    std::ifstream file(std::string(SHADER_PATH) + "/" + shaderName + ".spv", std::ios::ate | std::ios::binary);

    assert(file.is_open());

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> shader(fileSize);

    file.seekg(0);
    file.read(shader.data(), fileSize);

    file.close();

    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eShaderModuleCreateInfo;
    createInfo.codeSize = shader.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(shader.data());

    return device.createShaderModule(createInfo);
}
