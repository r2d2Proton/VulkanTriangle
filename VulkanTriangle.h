#pragma once

#pragma comment(lib, "vulkan-1.lib")
#pragma comment(lib, "glfw3.lib")

#define NOMINMAX

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

#include <cstdint>
#include <limits>
#include <algorithm>

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


struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR caps;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};


class VulkanTriangleApp
{
public:

    void run();

protected:

    void initWindow();

    void initVulkan();
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();

    void mainLoop();

    void cleanUp();


    // createInstance
    bool checkValidationLayerSupport();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    std::vector<const char*> getRequiredExtensions();

    // setupDebugMessenger::populateDebugMessengerCreateInfo

    // createSurface

    // pickPhysicalDevice
    bool isDeviceSuitable(const VkPhysicalDevice& physicalDevice, const LogProfile& logProfile);
    uint32_t rateDeviceSuitability(const VkPhysicalDevice& physicalDevice, const LogProfile& logProfile);
    bool checkDeviceExtensionSupport(const VkPhysicalDevice& physicalDevice, const LogProfile& logProfile);
    QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& physicalDevice, const LogProfile& logProfile);

    // createLogicalDevice
    void createGraphicsQueue(const QueueFamilyIndices& queueIndices);
    void createComputeQueue(const QueueFamilyIndices& queueIndices);
    void createXferQueue(const QueueFamilyIndices& queueIndices);

    // createSwapChain
    SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice& physicalDevice, const LogProfile& logProfile);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& caps);

    // createImageViews

    // createRenderPass

    // createGraphicsPipeline
    VkShaderModule createShaderModule(const std::vector<unsigned char>& shaderCode);
    void enableAlphaBlending(VkPipelineColorBlendAttachmentState& colorBlendAttachmentState);
    void disableAlphaBlending(VkPipelineColorBlendAttachmentState& colorBlendAttachmentState);

    // createFramebuffers

    // createCommandPool


    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback
    (
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    );

private:

    GLFWwindow* pWindow = nullptr;
    VkInstance pInstance = nullptr;
    VkDebugUtilsMessengerEXT pDebugMessenger = nullptr;

    VkPhysicalDevice pPhysicalDevice = nullptr;

    VkDevice pDevice = nullptr;

    VkSurfaceKHR pSurface = nullptr;
    VkSwapchainKHR pSwapChain = nullptr;

    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkFormat swapChainImageFormat;
    VkColorSpaceKHR swapChainColorSpace;
    VkExtent2D swapChainExtent;

    VkRenderPass pRenderPass = nullptr;
    VkPipelineLayout pPipelineLayout = nullptr;
    VkPipeline pGraphicsPipeline = nullptr;

    VkCommandPool pCommandPool = nullptr;

    VkQueue pPresentQueue = nullptr;
    VkQueue pGraphicsQueue = nullptr;
    VkQueue pComputeQueue = nullptr;
    VkQueue pXferQueue = nullptr;

    QueueFamilyIndices queueFamilyIndices;
};
