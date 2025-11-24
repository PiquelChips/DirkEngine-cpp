#include "render/renderer.hpp"
#include "asserts.hpp"
#include "backends/imgui_impl_vulkan.h"
#include "common.hpp"
#include "engine/dirkengine.hpp"
#include "engine/world.hpp"
#include "imgui.h"
#include "logging/logging.hpp"
#include "platform/platform.hpp"
#include "render/camera.hpp"
#include "render/viewport.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "resources/resource_manager.hpp"
#include "tinygltf.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
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

static void checkVkResult(VkResult err) {
    if (err == VK_SUCCESS)
        return;
    DIRK_LOG(LogVulkan, ERROR, "vk::Result = " << err);
}

DEFINE_LOG_CATEGORY(LogVulkan)
DEFINE_LOG_CATEGORY(LogVulkanValidation)
DEFINE_LOG_CATEGORY(LogRenderer)

Renderer::Renderer() {
    DIRK_LOG(LogVulkan, INFO, "initlializing Vulkan...");

#ifdef ENABLE_VALIDATION_LAYERS
    // debug messenger
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{};
    debugUtilsMessengerCreateInfoEXT.messageSeverity = severityFlags;
    debugUtilsMessengerCreateInfoEXT.messageType = messageTypeFlags;
    debugUtilsMessengerCreateInfoEXT.pfnUserCallback = &debugCallback;
#endif

    vk::ApplicationInfo appInfo{};
    appInfo.sType = vk::StructureType::eApplicationInfo;
    appInfo.pApplicationName = "DirkEngine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "DirkEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = vk::ApiVersion14;

    vk::InstanceCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eInstanceCreateInfo;
    createInfo.pApplicationInfo = &appInfo;

    std::vector<const char*> extentions{ vk::KHRSurfaceExtensionName };
#ifdef PLATFORM_LINUX
    extentions.push_back(vk::KHRWaylandSurfaceExtensionName);
#endif
#ifdef ENABLE_VALIDATION_LAYERS
    extentions.push_back(vk::EXTDebugUtilsExtensionName);

    check(checkValidationLayerSupport());
    DIRK_LOG(LogVulkan, INFO, "using validation layers");
    createInfo.enabledLayerCount = validationLayers.size();
    createInfo.ppEnabledLayerNames = validationLayers.data();
    createInfo.pNext = debugUtilsMessengerCreateInfoEXT;
#else
    createInfo.enabledLayerCount = 0;
#endif
    check(checkRequiredInstanceExtensions(extentions));

    createInfo.enabledExtensionCount = extentions.size();
    createInfo.ppEnabledExtensionNames = extentions.data();

    this->instance = vk::createInstance(createInfo);

#ifdef ENABLE_VALIDATION_LAYERS
    vk::detail::DispatchLoaderDynamic dispatcher(instance, vkGetInstanceProcAddr);
    this->debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT, nullptr, dispatcher);
#endif
}

void Renderer::init() {
    vk::SurfaceKHR surface = gEngine->getPlatform()->createTempSurface(instance);

    // PHYSICAL DEVICE
    {
        auto physicalDevices = instance.enumeratePhysicalDevices();

        // rank each available device
        std::multimap<int, vk::PhysicalDevice> candidates;

        for (const auto& device : physicalDevices) {
            int score = getDeviceSuitability(device, surface);
            candidates.insert(std::make_pair(score, device));
        }

        if (candidates.rbegin()->first > 0) {
            this->physicalDevice = candidates.rbegin()->second;
        } else {
            DIRK_LOG(LogVulkan, FATAL, "could not find a physical device");
            return;
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

        auto features = getDeviceFeatures(physicalDevice);
        auto formats = physicalDevice.getSurfaceFormatsKHR(surface);
        auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
        auto presentModes = physicalDevice.getSurfacePresentModesKHR(surface);

        this->properties.msaaSamples = features.msaaSamples;
        this->properties.anisotropy = features.anisotropy;
        this->properties.surfaceFormat = chooseSwapSurfaceFormat(formats);
        this->properties.minImageCount = capabilities.minImageCount;
        this->properties.msaaSamples = getMaxUsableSampleCount(physicalDevice);
        this->properties.queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

        this->properties.depthFormat = findSupportedFormat(
            physicalDevice,
            { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment);
    }

    // LOGICAL DEVICE
    {

        // queues
        std::set<uint32_t> uniqueQueueFamilies = { properties.queueFamilyIndices.graphicsFamily.value(), properties.queueFamilyIndices.presentFamily.value() };
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos(uniqueQueueFamilies.size());
        float queuePriority = 1.f;
        for (int i = 0; i < uniqueQueueFamilies.size(); i++) {
            std::set<uint32_t>::iterator iter = uniqueQueueFamilies.find(i);
            if (iter == uniqueQueueFamilies.end()) {
                DIRK_LOG(LogVulkan, FATAL, "error creating logical device");
            }

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

        vk::PhysicalDeviceVulkan13Features vulkanFeatures{};
        vulkanFeatures.dynamicRendering = vk::True;
        createInfo.pNext = vulkanFeatures;

        this->device = physicalDevice.createDevice(createInfo);

        this->queues.graphicsQueue = device.getQueue(properties.queueFamilyIndices.graphicsFamily.value(), 0);
        this->queues.presentQueue = device.getQueue(properties.queueFamilyIndices.presentFamily.value(), 0);

        vk::FenceCreateInfo fenceInfo{};
        fenceInfo.sType = vk::StructureType::eFenceCreateInfo;
        fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled; // create the fence as signaled to avoid stalling at first draw call
        this->inFlightFence = device.createFence(fenceInfo);
    }

    // COMMAND POOL
    {
        vk::CommandPoolCreateInfo poolInfo{};
        poolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
        poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        poolInfo.queueFamilyIndex = properties.queueFamilyIndices.graphicsFamily.value();

        this->commandPool = device.createCommandPool(poolInfo);
    }

    // DESCIPTOR SETS
    {
        std::array bindings = {
            vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr),
            vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr),
        };

        vk::DescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.bindingCount = bindings.size();
        layoutInfo.pBindings = bindings.data();
        this->descriptorSetLayout = device.createDescriptorSetLayout(layoutInfo);

        std::array poolSizes{
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, MAX_DESCRIPTOR_SET_COUNT),
            vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, MAX_DESCRIPTOR_SET_COUNT),
        };
        vk::DescriptorPoolCreateInfo poolInfo{};
        poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        poolInfo.maxSets = MAX_DESCRIPTOR_SET_COUNT;
        poolInfo.poolSizeCount = poolSizes.size();
        poolInfo.pPoolSizes = poolSizes.data();

        this->descriptorPool = device.createDescriptorPool(poolInfo);
    }

    DIRK_LOG(LogVulkan, INFO, "vulkan initialized successfully");

    // ImGui
    {
        DIRK_LOG(LogVulkan, INFO, "initlializing imgui");
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui::StyleColorsDark();

        // Setup scaling
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(1.f);
        style.FontScaleDpi = 1.f;
        io.ConfigDpiScaleFonts = true;
        io.ConfigDpiScaleViewports = true;
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;

        // TODO: have these be engine functions. renderer should not interact with platform
        gEngine->getPlatform()->initImGui();
        auto mainWindow = &gEngine->getPlatform()->getMainWindow();
        check(mainWindow);
        mainWindow->show();

        auto formats = physicalDevice.getSurfaceFormatsKHR(surface);
        auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
        auto swapChainImageFormat = chooseSwapSurfaceFormat(formats);

        vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
        pipelineRenderingCreateInfo.colorAttachmentCount = 1;
        pipelineRenderingCreateInfo.pColorAttachmentFormats = &swapChainImageFormat.format;
        pipelineRenderingCreateInfo.depthAttachmentFormat = properties.depthFormat;

        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance = instance;
        initInfo.PhysicalDevice = physicalDevice;
        initInfo.Device = device;
        initInfo.QueueFamily = properties.queueFamilyIndices.graphicsFamily.value();
        initInfo.Queue = queues.graphicsQueue;
        initInfo.DescriptorPoolSize = MAX_DESCRIPTOR_SET_COUNT;
        initInfo.MinImageCount = capabilities.minImageCount;
        initInfo.ImageCount = mainWindow->getImageCount();
        initInfo.Allocator = nullptr;
        initInfo.PipelineInfoMain.Subpass = 0;
        initInfo.PipelineInfoMain.MSAASamples = (VkSampleCountFlagBits) vk::SampleCountFlagBits::e1;
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = pipelineRenderingCreateInfo;
        initInfo.CheckVkResultFn = checkVkResult;
        initInfo.UseDynamicRendering = true;
        ImGui_ImplVulkan_Init(&initInfo);
        DIRK_LOG(LogRenderer, INFO, "initlialized ImGui")
    }
}

Renderer::~Renderer() {
    // make sure all device ops are finished
    device.waitIdle();
    DIRK_LOG(LogVulkan, INFO, "cleaning up renderer");
}

void Renderer::render() {
    checkVulkan(device.waitForFences(1, &inFlightFence, vk::True, UINT64_MAX));
    checkVulkan(device.resetFences(1, &inFlightFence));

    // render engine viewports
    std::vector<vk::SubmitInfo> submitInfos(viewports.size());
    for (auto& viewport : viewports) {
        submitInfos.emplace_back(viewport->render());
    }

    checkVulkan(queues.graphicsQueue.submit(submitInfos.size(), submitInfos.data(), inFlightFence));

    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();

    // TODO: process all ImGui rendering
    ImGui::ShowDemoWindow();

    ImGui::Render();

    checkVulkan(device.waitForFences(1, &inFlightFence, vk::True, UINT64_MAX));
    checkVulkan(device.resetFences(1, &inFlightFence));

    auto window = &gEngine->getPlatform()->getMainWindow();
    queues.graphicsQueue.submit(window->render(ImGui::GetDrawData()), inFlightFence);

    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();

    checkVulkan(queues.presentQueue.presentKHR(window->present()));
}

std::shared_ptr<Viewport> Renderer::createViewport(const ViewportCreateInfo& createInfo) {
    return viewports.emplace_back(std::make_shared<Viewport>(createInfo));
}

void Renderer::destroyViewport(std::shared_ptr<Viewport> viewport) {
    viewports.erase(std::find(viewports.begin(), viewports.end(), viewport));
}

std::vector<SwapchainImage> Renderer::createSwapChain(const SwapChainCreateInfo& createInfo) {
    auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(createInfo.surface);
    auto oldSwapchain = createInfo.swapChain;

    vk::Extent2D extent = chooseSwapExtent(createInfo.windowSize, capabilities);
    uint32_t imageCount = capabilities.minImageCount + 1;
    createInfo.swapChainExtent = extent;

    // 0 means no limit to image count
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
        imageCount = capabilities.maxImageCount;

    vk::SwapchainCreateInfoKHR swapCreateInfo{};
    swapCreateInfo.sType = vk::StructureType::eSwapchainCreateInfoKHR;
    swapCreateInfo.surface = createInfo.surface;

    // the details and capabilities we selected
    swapCreateInfo.minImageCount = imageCount;
    swapCreateInfo.imageFormat = createInfo.surfaceFormat.format;
    swapCreateInfo.imageColorSpace = createInfo.surfaceFormat.colorSpace;
    swapCreateInfo.imageExtent = createInfo.swapChainExtent;
    swapCreateInfo.presentMode = createInfo.presentMode;

    // other settings
    swapCreateInfo.imageArrayLayers = 1;
    swapCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    swapCreateInfo.preTransform = capabilities.currentTransform;
    swapCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque; // ignore alpha
    swapCreateInfo.clipped = vk::True;                                      // ingore hidden pixels (behind other windows for ex)
    swapCreateInfo.oldSwapchain = oldSwapchain;

    // image sharing if multiple queues
    QueueFamilyIndices indices = properties.queueFamilyIndices;
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        swapCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        swapCreateInfo.queueFamilyIndexCount = 2;
        swapCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
        swapCreateInfo.queueFamilyIndexCount = 0;
        swapCreateInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.swapChain = device.createSwapchainKHR(swapCreateInfo);
    check(createInfo.swapChain);
    std::vector<vk::Image> images = device.getSwapchainImagesKHR(createInfo.swapChain);

    std::vector<SwapchainImage> swapImages(images.size());

    for (int i = 0; i < images.size(); i++) {
        auto commandBuffer = beginSingleTimeCommands();
        transitionImageLayout(commandBuffer, images[i], createInfo.surfaceFormat.format, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);
        endSingleTimeCommands(commandBuffer, queues.graphicsQueue);

        swapImages[i] = SwapchainImage{
            .image = images[i],
            .view = createImageView(images[i], createInfo.surfaceFormat.format),
        };
    }

    return swapImages;
}

vk::Semaphore Renderer::createSemaphore() {
    vk::SemaphoreCreateInfo createInfo{};
    return device.createSemaphore(createInfo);
}

vk::CommandBuffer Renderer::createCommandBuffer() {
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.commandBufferCount = 1;
    allocInfo.commandPool = commandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;

    return device.allocateCommandBuffers(allocInfo)[0];
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

int Renderer::getDeviceSuitability(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
    check(device);

    vk::PhysicalDeviceProperties deviceProperties = device.getProperties();
    vk::PhysicalDeviceFeatures deviceFeatures = device.getFeatures();

    // TODO: update with vulkan tutorial checks

    // prereturn required stuff
    if (!deviceFeatures.geometryShader)
        return 0;

    QueueFamilyIndices indices = findQueueFamilies(device, surface);
    if (!indices.isComplete())
        return 0;

    if (!checkDeviceExtensionSupport(device))
        return 0;

    auto presentModes = device.getSurfacePresentModesKHR(surface);
    auto formats = device.getSurfaceFormatsKHR(surface);
    if (formats.empty() || presentModes.empty())
        return 0;

    // calculate a score to create preference based on device
    int score = 0;

    if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
        score += 1000;

    score += deviceProperties.limits.maxImageDimension2D;

    if (indices.presentFamily == indices.graphicsFamily)
        score += 10;

    score += formats.size();
    score += presentModes.size();

    score += getDeviceFeatures(device).getScore();

    DIRK_LOG(LogVulkan, DEBUG, "found device: " << deviceProperties.deviceName << "(score: " << score << ")");
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

vk::Extent2D Renderer::chooseSwapExtent(vk::Extent2D windowSize, const vk::SurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;

    windowSize.width = std::clamp(windowSize.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    windowSize.height = std::clamp(windowSize.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return windowSize;
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
#endif

ImageMemoryView Renderer::createImageMemoryView(CreateImageMemoryViewInfo& createInfo) {
    check(createInfo.format != vk::Format::eUndefined);
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
    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;
    poolInfo.queueFamilyIndex = properties.queueFamilyIndices.graphicsFamily.value();

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
    } else if (oldLayout == vk::ImageLayout::ePresentSrcKHR && newLayout == vk::ImageLayout::eColorAttachmentOptimal) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

        sourceStage = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eNone;
        destinationStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    } else if (oldLayout == vk::ImageLayout::eColorAttachmentOptimal && newLayout == vk::ImageLayout::ePresentSrcKHR) {
        barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        barrier.dstAccessMask = {};

        sourceStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        destinationStage = vk::PipelineStageFlagBits::eBottomOfPipe;
    } else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::ePresentSrcKHR) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = {};

        sourceStage = vk::PipelineStageFlagBits::eBottomOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
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

DeviceFeatures Renderer::getDeviceFeatures(vk::PhysicalDevice physicalDevice) {
    vk::PhysicalDeviceFeatures deviceFeatures = physicalDevice.getFeatures();
    return DeviceFeatures{
        .anisotropy = deviceFeatures.samplerAnisotropy == vk::True,
        .msaaSamples = getMaxUsableSampleCount(physicalDevice),
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

QueueFamilyIndices Renderer::findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
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
