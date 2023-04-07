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

const vector<const char*> validationLayers =
{
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif


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
}


void VulkanTriangleApp::createSurface()
{
    if (glfwCreateWindowSurface(pInstance, pWindow, nullptr, &pSurface) != VK_SUCCESS)
        throw runtime_error("failed to create window surface");
}


bool VulkanTriangleApp::isDeviceSuitable(const VkPhysicalDevice& device, const LogProfile& logProfile)
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

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
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    if (logProfile.logFeatures) {
        Logging::logDeviceFeatures(deviceFeatures);
    }

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

    queueFamilyIndices = findQueueFamilies(physicalDevice, logProfile);

    // need geometry shader
    if (!deviceFeatures.geometryShader)
        return 0;

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

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queuesCreateInfo.size());
    deviceCreateInfo.pQueueCreateInfos = queuesCreateInfo.data();

    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    if (enableValidationLayers) {
        deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
    }

    if (vkCreateDevice(pPhysicalDevice, &deviceCreateInfo, nullptr, &pDevice) != VK_SUCCESS)
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
    vkDestroyDevice(pDevice, nullptr);

    if (enableValidationLayers)
        DestroyDebugUtilsMessengerEXT(pInstance, pDebugMessenger, nullptr);

    vkDestroySurfaceKHR(pInstance, pSurface, nullptr);
    vkDestroyInstance(pInstance, nullptr);

    glfwDestroyWindow(pWindow);
    glfwTerminate();
}
