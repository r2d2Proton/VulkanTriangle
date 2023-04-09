#pragma once
#include <vulkan/vulkan.h>

#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <format>
#include <string>


struct LogProfile
{
    bool logAll = true;

    bool logProps = true;
    bool logLimits = true;
    bool logSparseProps = true;

    bool logFeatures = true;

    bool loqGraphicsQueue = true;
    bool logComputeQueue = true;
    bool logXferQueue = true;

    bool logCaps = true;
    bool logFormats = true;
    bool logPresentModes = true;

    bool logExtensions = true;

    //:
};

namespace Logging
{
    std::string FormatLog(std::string msg);

    void logDeviceProps(const VkPhysicalDeviceProperties& deviceProperties);
    void logDeviceLimits(const VkPhysicalDeviceLimits& limits);
    void logDeviceSparseProps(const VkPhysicalDeviceSparseProperties& sparseProps);
    void logDeviceFeatures(const VkPhysicalDeviceFeatures& deviceFeatures);

    void logDeviceQueueFamily(const std::string& name, const VkQueueFamilyProperties& queueFamily);

    void logSurfaceCapabilities(const VkSurfaceCapabilitiesKHR& caps);
    void logSurfaceFormats(std::vector<VkSurfaceFormatKHR> formats);
    void logPresentModes(std::vector<VkPresentModeKHR> presentModes);

    void logDeviceExtensions(std::vector<VkExtensionProperties> availableExtensions);
}
