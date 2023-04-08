#include "VulkanTriangle.h"

using std::optional;
using std::string;
using std::vector;
using std::multimap;
using std::set;

using std::endl;
using std::cerr;
using std::boolalpha;
using std::noboolalpha;
using std::runtime_error;
using std::exception;

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const vector<const char*> validationLayers =
{
    "VK_LAYER_KHRONOS_validation"
};

const vector<const char*> deviceExtensions =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


VkResult CreateDebugUtilsMessengerEXT
(
    VkInstance pInstance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger
)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(pInstance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
        return func(pInstance, pCreateInfo, pAllocator, pDebugMessenger);

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}


void DestroyDebugUtilsMessengerEXT(VkInstance pInstance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(pInstance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
        func(pInstance, debugMessenger, pAllocator);
}


void VulkanTriangleApp::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanUp();
}


void VulkanTriangleApp::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    pWindow = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}


void VulkanTriangleApp::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
}


void VulkanTriangleApp::createSurface()
{
    if (glfwCreateWindowSurface(pInstance, pWindow, nullptr, &pSurface) != VK_SUCCESS)
        throw runtime_error("failed to create window surface");
}


SwapChainSupportDetails VulkanTriangleApp::querySwapChainSupport(const VkPhysicalDevice& physicalDevice, const LogProfile& logProfile)
{
    // min/max swapchain images, min/max image width/height
    // surface formats (pixel format, colorspace)
    // presentation modes
    SwapChainSupportDetails swapChainDetails{};

    // query surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, pSurface, &swapChainDetails.caps);

    // query supported surface formats
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, pSurface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        swapChainDetails.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, pSurface, &formatCount, swapChainDetails.formats.data());
    }

    // query presentation modes
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, pSurface, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
        swapChainDetails.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, pSurface, &presentModeCount, swapChainDetails.presentModes.data());
    }

    if (logProfile.logCaps)
        Logging::logSurfaceCapabilities(swapChainDetails.caps);

    if (logProfile.logFormats)
        Logging::logSurfaceFormats(swapChainDetails.formats);

    if (logProfile.logPresentModes)
        Logging::logPresentModes(swapChainDetails.presentModes);

    return swapChainDetails;
}


// surface format (color depth)
// presentation mode (swapping)
// swap extent (resolution)
VkSurfaceFormatKHR VulkanTriangleApp::chooseSwapSurfaceFormat(const vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;
    }

    return availableFormats[0];
}


// VK_PRESENT_MODE_IMMEDIATE_KHR           possible tearing
// VK_PRESENT_MODE_FIFO_KHR                swapchain queue images taken from front of the queue - program waits if queue is full (guaranteed)
// VK_PRESENT_MODE_FIFO_RELEAXED_KHR
// VK_PRESENT_MODE_MAILBOX_KHR             can render frames as fast as possible without tearing with fewer vsync latency issues
VkPresentModeKHR VulkanTriangleApp::chooseSwapPresentMode(const vector<VkPresentModeKHR>& availablePresentModes)
{
#ifdef WIN32
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return availablePresentMode;
    }
#endif

    return VK_PRESENT_MODE_FIFO_KHR;
}


VkExtent2D VulkanTriangleApp::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& caps)
{
    if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return caps.currentExtent;

    int width = 0, height = 0;
    glfwGetFramebufferSize(pWindow, &width, &height);

    VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    actualExtent.width = std::clamp(actualExtent.width, caps.minImageExtent.width, caps.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, caps.minImageExtent.height, caps.maxImageExtent.height);

    return actualExtent;
}


void VulkanTriangleApp::createSwapChain()
{
    LogProfile logProfile;
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(pPhysicalDevice, logProfile);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.caps);

    uint32_t imageCount = swapChainSupport.caps.minImageCount + 1;

    // 0 maxImageCount means no maximum
    if (swapChainSupport.caps.maxImageCount > 0 && imageCount > swapChainSupport.caps.maxImageCount)
        imageCount = swapChainSupport.caps.maxImageCount;

    VkSwapchainCreateInfoKHR swapChainCreateInfo{};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = pSurface;
    swapChainCreateInfo.minImageCount = imageCount;
    swapChainCreateInfo.imageFormat = surfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapChainCreateInfo.imageExtent = extent;
    swapChainCreateInfo.imageArrayLayers = 1; // unless stereoscopic 3D
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueIndices[] = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value() };

    if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily)
    {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueIndices;
    }
    else
    {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCreateInfo.queueFamilyIndexCount = 0;
        swapChainCreateInfo.pQueueFamilyIndices = nullptr;
    }

    swapChainCreateInfo.preTransform = swapChainSupport.caps.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.presentMode = presentMode;
    swapChainCreateInfo.clipped = VK_TRUE;
    swapChainCreateInfo.oldSwapchain = nullptr;

    if (vkCreateSwapchainKHR(pDevice, &swapChainCreateInfo, nullptr, &pSwapChain) != VK_SUCCESS)
        throw runtime_error("failed to create swapchain");

    vkGetSwapchainImagesKHR(pDevice, pSwapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(pDevice, pSwapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainColorSpace = surfaceFormat.colorSpace;
    swapChainExtent = extent;
}


bool VulkanTriangleApp::isDeviceSuitable(const VkPhysicalDevice& physicalDevice, const LogProfile& logProfile)
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

    if (logProfile.logProps) {
        Logging::logDeviceProps(deviceProperties);
    }

    if (logProfile.logLimits) {
        Logging::logDeviceLimits(deviceProperties.limits);
    }

    if (logProfile.logSparseProps) {
        Logging::logDeviceSparseProps(deviceProperties.sparseProperties);
    }

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    if (logProfile.logFeatures) {
        Logging::logDeviceFeatures(deviceFeatures);
    }

    // need geometry shader
    if (!deviceFeatures.geometryShader)
        return false;

    // need swapchain
    if (!checkDeviceExtensionSupport(physicalDevice, logProfile))
        return false;

    bool swapChainAdequate = false;
    SwapChainSupportDetails swapChainDetails = querySwapChainSupport(physicalDevice, logProfile);
    swapChainAdequate = !swapChainDetails.formats.empty() && !swapChainDetails.presentModes.empty();

    if (!swapChainAdequate)
        return false;

    queueFamilyIndices = findQueueFamilies(physicalDevice, logProfile);

    // need graphics queue
    if (!queueFamilyIndices.HasGraphicsQueue())
        return false;

    // need present queue
    if (!queueFamilyIndices.HasPresentQueue())
        return false;

    // need compute queue
    if (!queueFamilyIndices.HasComputeQueue())
        return false;

    return true;
}


QueueFamilyIndices VulkanTriangleApp::findQueueFamilies(const VkPhysicalDevice& physicalDevice, const LogProfile& logProfile)
{
    QueueFamilyIndices queueIndices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queueIndices.graphicsFamily = i;

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, pSurface, &presentSupport);

            if (presentSupport)
                queueIndices.presentFamily = i;

            if (logProfile.loqGraphicsQueue)
                Logging::logDeviceQueueFamily("Graphics", queueFamily);
        }
        else if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            queueIndices.computeFamily = i;
            if (logProfile.logComputeQueue)
                Logging::logDeviceQueueFamily("Compute", queueFamily);
        }
        else if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            queueIndices.xferFamily = i;
            if (logProfile.logXferQueue)
                Logging::logDeviceQueueFamily("Transfer", queueFamily);
        }

        ++i;
    }

    return queueIndices;
}


uint32_t VulkanTriangleApp::rateDeviceSuitability(const VkPhysicalDevice& physicalDevice, const LogProfile& logProfile)
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

    if (logProfile.logProps) {
        Logging::logDeviceProps(deviceProperties);
    }

    if (logProfile.logLimits) {
        Logging::logDeviceLimits(deviceProperties.limits);
    }

    if (logProfile.logSparseProps) {
        Logging::logDeviceSparseProps(deviceProperties.sparseProperties);
    }

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    if (logProfile.logFeatures) {
        Logging::logDeviceFeatures(deviceFeatures);
    }

    // need geometry shader
    if (!deviceFeatures.geometryShader)
        return 0;

    // need swapchain
    if (!checkDeviceExtensionSupport(physicalDevice, logProfile))
        return 0;

    bool swapChainAdequate = false;
    SwapChainSupportDetails swapChainDetails = querySwapChainSupport(physicalDevice, logProfile);
    swapChainAdequate = !swapChainDetails.formats.empty() && !swapChainDetails.presentModes.empty();

    if (!swapChainAdequate)
        return false;

    queueFamilyIndices = findQueueFamilies(physicalDevice, logProfile);

    // need graphics queue
    if (!queueFamilyIndices.HasGraphicsQueue())
        return 0;

    // need present queue
    if (!queueFamilyIndices.HasPresentQueue())
        return 0;

    // need compute queue
    if (!queueFamilyIndices.HasComputeQueue())
        return 0;

    uint32_t score = 0;

    // descrete GPUs are preferred
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        score += 1000;

    // maximum texture size affects image quality
    score += deviceProperties.limits.maxImageDimension2D;

    return score;
}


bool VulkanTriangleApp::checkDeviceExtensionSupport(const VkPhysicalDevice& physicalDevice, const LogProfile& logProfile)
{
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

    vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

    set<string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}


void VulkanTriangleApp::pickPhysicalDevice()
{
    LogProfile logProfile;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(pInstance, &deviceCount, nullptr);

    if (deviceCount == 0)
        throw runtime_error("failed to find GPUs with Vulkan support!");

    vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(pInstance, &deviceCount, devices.data());

    multimap<uint32_t, VkPhysicalDevice> candidates;
    for (const auto& device : devices)
    {
        uint32_t score = rateDeviceSuitability(device, logProfile);
        candidates.insert(std::make_pair(score, device));
    }

    // find the best candidate
    if (candidates.rbegin()->first > 0)
        pPhysicalDevice = candidates.rbegin()->second;

    if (pPhysicalDevice == VK_NULL_HANDLE)
        throw runtime_error("failed to find suitable GPU");
}


void VulkanTriangleApp::createLogicalDevice()
{
    LogProfile logProfile;

    createGraphicsQueue(queueFamilyIndices);
    createComputeQueue(queueFamilyIndices);
    createXferQueue(queueFamilyIndices);
}


void VulkanTriangleApp::createGraphicsQueue(const QueueFamilyIndices& queueIndices)
{
    // use set to make sure the same queue (graphics/present) are
    set<uint32_t> uniqueQueues = { queueIndices.graphicsFamily.value(), queueIndices.presentFamily.value() };
    vector<VkDeviceQueueCreateInfo> queuesCreateInfo;

    // share pointer to same priority but maybe change later
    for (uint32_t queueFamily : uniqueQueues)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queueIndices.graphicsQueuePriority;
        queuesCreateInfo.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures physicalDeviceFeatures{};

    VkDeviceCreateInfo logicalDeviceCreateInfo{};
    logicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    logicalDeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queuesCreateInfo.size());
    logicalDeviceCreateInfo.pQueueCreateInfos = queuesCreateInfo.data();

    logicalDeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    logicalDeviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    logicalDeviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

    if (enableValidationLayers) {
        logicalDeviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        logicalDeviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
    }

    if (vkCreateDevice(pPhysicalDevice, &logicalDeviceCreateInfo, nullptr, &pDevice) != VK_SUCCESS)
        throw runtime_error("failed to create logical device");

    // get graphics queue - logicalDevice, queueFamily, queueIndex, pHandle
    vkGetDeviceQueue(pDevice, queueIndices.graphicsFamily.value(), 0, &pGraphicsQueue);

    // get present queue - logicalDevice, queueFamily, queueIndex, pHandle
    vkGetDeviceQueue(pDevice, queueIndices.presentFamily.value(), 0, &pPresentQueue);
}


void VulkanTriangleApp::createComputeQueue(const QueueFamilyIndices& queueIndices)
{
}


void VulkanTriangleApp::createXferQueue(const QueueFamilyIndices& queueIndices)
{
}


VKAPI_ATTR VkBool32 VKAPI_CALL VulkanTriangleApp::debugCallback
(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
)
{
    cerr << "validation layer: " << pCallbackData->pMessage << endl;
    return VK_FALSE;
}


void VulkanTriangleApp::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = this;
}


void VulkanTriangleApp::setupDebugMessenger()
{
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(pInstance, &createInfo, nullptr, &pDebugMessenger) != VK_SUCCESS)
    {
        throw runtime_error("failed to set up debug messenger");
    }
}


bool VulkanTriangleApp::checkValidationLayerSupport()
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (layerFound)
            return true;
    }

    return false;
}


vector<const char*> VulkanTriangleApp::getRequiredExtensions()
{
    uint32_t extCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extCount);
    vector<const char*> extensions(glfwExtensions, glfwExtensions + extCount);

    if (enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}


void VulkanTriangleApp::createInstance()
{
    if (enableValidationLayers && !checkValidationLayerSupport())
    {
        throw runtime_error("validation layers requsted, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;


    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (vkCreateInstance(&createInfo, nullptr, &pInstance) != VK_SUCCESS)
    {
        throw runtime_error("failed to create instance");
    }
}


void VulkanTriangleApp::mainLoop()
{
    while (!glfwWindowShouldClose(pWindow))
    {
        glfwPollEvents();
    }
}


void VulkanTriangleApp::cleanUp()
{
    vkDestroySwapchainKHR(pDevice, pSwapChain, nullptr);
    vkDestroyDevice(pDevice, nullptr);

    if (enableValidationLayers)
        DestroyDebugUtilsMessengerEXT(pInstance, pDebugMessenger, nullptr);

    vkDestroySurfaceKHR(pInstance, pSurface, nullptr);
    vkDestroyInstance(pInstance, nullptr);

    glfwDestroyWindow(pWindow);
    glfwTerminate();
}
