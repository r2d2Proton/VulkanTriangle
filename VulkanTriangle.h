#pragma once

#pragma comment(lib, "vulkan-1.lib")
#pragma comment(lib, "glfw3.lib")

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <vulkan/vulkan.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>

#include <vector>
#include <set>
#include <map>
#include <optional>

#include "Logging.h"


struct QueueFamilyIndices
{
    std::optional<uint32_t> presentFamily;

    // [0..1]
    float graphicsQueuePriority = 1.0f;
    std::optional<uint32_t> graphicsFamily;

    // [0..1]
    float computeQueuePriority = 1.0f;
    std::optional<uint32_t> computeFamily;

    // [0..1]
    float xferQueuePriority = 1.0f;
    std::optional<uint32_t> xferFamily;

    bool HasPresentQueue() { return presentFamily.has_value(); }
    bool HasGraphicsQueue() { return graphicsFamily.has_value(); }
    bool HasComputeQueue() { return computeFamily.has_value(); }
    bool HasXferQueue() { return xferFamily.has_value(); }
};


class VulkanTriangleApp
{
public:

    void run();

protected:

    void initWindow();

    void initVulkan();

    void createSurface();

    bool isDeviceSuitable(const VkPhysicalDevice& physicalDevice, const LogProfile& logProfile);

    QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& physicalDevice, const LogProfile& logProfile);

    uint32_t rateDeviceSuitability(const VkPhysicalDevice& physicalDevice, const LogProfile& logProfile);

    void pickPhysicalDevice();

    void createLogicalDevice();
    void createPresentQueue(const QueueFamilyIndices& queueIndices);
    void createGraphicsQueue(const QueueFamilyIndices& queueIndices);
    void createComputeQueue(const QueueFamilyIndices& queueIndices);
    void createXferQueue(const QueueFamilyIndices& queueIndices);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback
    (
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    );

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    void setupDebugMessenger();

    bool checkValidationLayerSupport();

    std::vector<const char*> getRequiredExtensions();

    void createInstance();

    void mainLoop();

    void cleanUp();

private:

    GLFWwindow* pWindow = nullptr;
    VkInstance pInstance = nullptr;
    VkDebugUtilsMessengerEXT pDebugMessenger = nullptr;

    VkPhysicalDevice pPhysicalDevice = nullptr;

    VkDevice pDevice = nullptr;

    VkSurfaceKHR pSurface = nullptr;

    VkQueue pPresentQueue = nullptr;
    VkQueue pGraphicsQueue = nullptr;
    VkQueue pComputeQueue = nullptr;
    VkQueue pXferQueue = nullptr;

    QueueFamilyIndices queueFamilyIndices;
};
