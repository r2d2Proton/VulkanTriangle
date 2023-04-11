#pragma once
#include <vulkan/vulkan.h>

#include <string>
#include <vector>

namespace Utils
{
    VkResult CreateDebugUtilsMessengerEXT
    (
        VkInstance pInstance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger
    );

    void DestroyDebugUtilsMessengerEXT(VkInstance pInstance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);


    std::vector<unsigned char> readFile(const std::string& filename);
}
