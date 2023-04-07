#pragma comment(lib, "vulkan-1.lib")
#pragma comment(lib, "glfw3.lib")

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>

#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <format>
#include <string>
#include <vector>
#include <map>
#include <optional>

using std::string;
using std::vector;
using std::multimap;
using std::optional;

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

struct QueueFamilyIndices
{
    optional<uint32_t> graphicsFamily;
    optional<uint32_t> computeFamily;
    optional<uint32_t> xferFamily;

    bool HasGraphicsQueue() { return graphicsFamily.has_value(); }
    bool HasComputeQueue() { return computeFamily.has_value(); }
    bool HasXferQueue() { return xferFamily.has_value(); }
};

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
    //:
};

string FormatLog(string msg)
{
    auto datetime = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
    string output = format("{0:%F} {0:%X} : {1}", datetime, msg);
    return output;
}

VkResult CreateDebugUtilsMessengerEXT
(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger
)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}


void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
        func(instance, debugMessenger, pAllocator);
}


class HelloTriangleApp
{
public:

    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanUp();
    }

private:

    GLFWwindow* pWindow = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;


    void initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        pWindow = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan()
    {
        createInstance();
        setupDebugMessenger();
        pickPhysicalDevice();
    }

    void logDeviceProps(const VkPhysicalDeviceProperties& deviceProperties)
    {
        cerr << "PhysicalDevice Name: " << deviceProperties.deviceName << endl;
        cerr << "PhysicalDevice VenderID: " << deviceProperties.vendorID << endl;
        cerr << "PhysicalDevice DeviceID: " << deviceProperties.deviceID << endl;
        cerr << "PhysicalDevice DeviceType: " << deviceProperties.deviceType << endl;
        cerr << "PhysicalDevice Driver Version: " << deviceProperties.driverVersion << endl;
        cerr << "PhysicalDevice API Version: " << deviceProperties.apiVersion << endl;
        cerr << "PhysicalDevice Pipeline Cache UUID: " << deviceProperties.pipelineCacheUUID << endl;
    }

    void logDeviceLimits(const VkPhysicalDeviceLimits& limits)
    {
        cerr << "PhysicalDevice Limits" << endl;
        cerr << "\tmaxImageDimension1D: " << limits.maxImageDimension1D << endl;
        cerr << "\tmaxImageDimension2D: " << limits.maxImageDimension2D << endl;
        cerr << "\tmaxImageDimension3D: " << limits.maxImageDimension3D << endl;
        cerr << "\tmaxImageDimensionCube: " << limits.maxImageDimensionCube << endl;
        cerr << "\tmaxImageArrayLayers: " << limits.maxImageArrayLayers << endl;
        cerr << "\tmaxTexelBufferElements: " << limits.maxTexelBufferElements << endl;
        cerr << "\tmaxUniformBufferRange: " << limits.maxUniformBufferRange << endl;
        cerr << "\tmaxStorageBufferRange: " << limits.maxStorageBufferRange << endl;
        cerr << "\tmaxPushConstantsSize: " << limits.maxPushConstantsSize << endl;
        cerr << "\tmaxMemoryAllocationCount: " << limits.maxMemoryAllocationCount << endl;
        cerr << "\tmaxSamplerAllocationCount: " << limits.maxSamplerAllocationCount << endl;
        cerr << "\tbufferImageGranularity: " << limits.bufferImageGranularity << endl;
        cerr << "\tsparseAddressSpaceSize: " << limits.sparseAddressSpaceSize << endl;
        cerr << "\tmaxBoundDescriptorSets: " << limits.maxBoundDescriptorSets << endl;
        cerr << "\tmaxPerStageDescriptorSamplers: " << limits.maxPerStageDescriptorSamplers << endl;
        cerr << "\tmaxPerStageDescriptorUniformBuffers: " << limits.maxPerStageDescriptorUniformBuffers << endl;
        cerr << "\tmaxPerStageDescriptorStorageBuffers: " << limits.maxPerStageDescriptorStorageBuffers << endl;
        cerr << "\tmaxPerStageDescriptorSampledImages: " << limits.maxPerStageDescriptorSampledImages << endl;
        cerr << "\tmaxPerStageDescriptorStorageImages: " << limits.maxPerStageDescriptorStorageImages << endl;
        cerr << "\tmaxPerStageDescriptorInputAttachments: " << limits.maxPerStageDescriptorInputAttachments << endl;
        cerr << "\tmaxPerStageResources: " << limits.maxPerStageResources << endl;
        cerr << "\tmaxDescriptorSetSamplers: " << limits.maxDescriptorSetSamplers << endl;
        cerr << "\tmaxDescriptorSetUniformBuffers: " << limits.maxDescriptorSetUniformBuffers << endl;
        cerr << "\tmaxDescriptorSetUniformBuffersDynamic: " << limits.maxDescriptorSetUniformBuffersDynamic << endl;
        cerr << "\tmaxDescriptorSetStorageBuffers: " << limits.maxDescriptorSetStorageBuffers << endl;
        cerr << "\tmaxDescriptorSetStorageBuffersDynamic: " << limits.maxDescriptorSetStorageBuffersDynamic << endl;
        cerr << "\tmaxDescriptorSetSampledImages: " << limits.maxDescriptorSetSampledImages << endl;
        cerr << "\tmaxDescriptorSetStorageImages: " << limits.maxDescriptorSetStorageImages << endl;
        cerr << "\tmaxDescriptorSetInputAttachments: " << limits.maxDescriptorSetInputAttachments << endl;
        cerr << "\tmaxVertexInputAttributes: " << limits.maxVertexInputAttributes << endl;
        cerr << "\tmaxVertexInputBindings: " << limits.maxVertexInputBindings << endl;
        cerr << "\tmaxVertexInputAttributeOffset: " << limits.maxVertexInputAttributeOffset << endl;
        cerr << "\tmaxVertexInputBindingStride: " << limits.maxVertexInputBindingStride << endl;
        cerr << "\tmaxVertexOutputComponents: " << limits.maxVertexOutputComponents << endl;
        cerr << "\tmaxTessellationGenerationLevel: " << limits.maxTessellationGenerationLevel << endl;
        cerr << "\tmaxTessellationPatchSize: " << limits.maxTessellationPatchSize << endl;
        cerr << "\tmaxTessellationControlPerVertexInputComponents: " << limits.maxTessellationControlPerVertexInputComponents << endl;
        cerr << "\tmaxTessellationControlPerVertexOutputComponents: " << limits.maxTessellationControlPerVertexOutputComponents << endl;
        cerr << "\tmaxTessellationControlPerPatchOutputComponents: " << limits.maxTessellationControlPerPatchOutputComponents << endl;
        cerr << "\tmaxTessellationControlTotalOutputComponents: " << limits.maxTessellationControlTotalOutputComponents << endl;
        cerr << "\tmaxTessellationEvaluationInputComponents: " << limits.maxTessellationEvaluationInputComponents << endl;
        cerr << "\tmaxTessellationEvaluationOutputComponents: " << limits.maxTessellationEvaluationOutputComponents << endl;
        cerr << "\tmaxGeometryShaderInvocations: " << limits.maxGeometryShaderInvocations << endl;
        cerr << "\tmaxGeometryInputComponents: " << limits.maxGeometryInputComponents << endl;
        cerr << "\tmaxGeometryOutputComponents: " << limits.maxGeometryOutputComponents << endl;
        cerr << "\tmaxGeometryOutputVertices: " << limits.maxGeometryOutputVertices << endl;
        cerr << "\tmaxGeometryTotalOutputComponents: " << limits.maxGeometryTotalOutputComponents << endl;
        cerr << "\tmaxFragmentInputComponents: " << limits.maxFragmentInputComponents << endl;
        cerr << "\tmaxFragmentOutputAttachments: " << limits.maxFragmentOutputAttachments << endl;
        cerr << "\tmaxFragmentDualSrcAttachments: " << limits.maxFragmentDualSrcAttachments << endl;
        cerr << "\tmaxFragmentCombinedOutputResources: " << limits.maxFragmentCombinedOutputResources << endl;
        cerr << "\tmaxComputeSharedMemorySize: " << limits.maxComputeSharedMemorySize << endl;

        cerr << "\tmaxComputeWorkGroupCount[" << _countof(limits.maxComputeWorkGroupCount) << "]" << endl;
        {
            uint32_t i = 0;
            for (uint32_t maxCompute : limits.maxComputeWorkGroupCount)
                cerr << "\t\tmaxComputeWorkGroupCount[" << i++ << "] = " << maxCompute << endl;
        }
        cerr << "\tmaxComputeWorkGroupInvocations: " << limits.maxComputeWorkGroupInvocations << endl;

        cerr << "\tmaxComputeWorkGroupSize[" << _countof(limits.maxComputeWorkGroupSize) << "]" << endl;
        {
            uint32_t i = 0;
            for (uint32_t maxSize : limits.maxComputeWorkGroupSize)
                cerr << "\t\tmaxComputeWorkGroupSize[" << i++ << "] = " << maxSize << endl;
        }

        cerr << "\tsubPixelPrecisionBits: " << limits.subPixelPrecisionBits << endl;
        cerr << "\tsubTexelPrecisionBits: " << limits.subTexelPrecisionBits << endl;
        cerr << "\tmipmapPrecisionBits: " << limits.mipmapPrecisionBits << endl;
        cerr << "\tmaxDrawIndirectCount: " << limits.maxDrawIndirectCount << endl;
        cerr << "\tmaxSamplerLodBias: " << limits.maxSamplerLodBias << endl;
        cerr << "\tmaxSamplerAnisotropy: " << limits.maxSamplerAnisotropy << endl;

        cerr << "\tmaxViewports: " << limits.maxViewports << endl;
        cerr << "\tmaxViewportDimensions[" << _countof(limits.maxViewportDimensions) <<"]" << endl;
        {
            uint32_t i = 0;
            for (uint32_t maxDim : limits.maxViewportDimensions)
                cerr << "\t\tmaxViewportDimensions[" << i++ << "] = " << maxDim << endl;
        }

        cerr << "\tviewportBoundsRange[" << _countof(limits.viewportBoundsRange) << "]" << endl;
        {
            uint32_t i = 0;
            for (float maxBounds : limits.viewportBoundsRange)
                cerr << "\t\tviewportBoundsRange[" << i++ << "] = " << maxBounds << endl;
        }

        cerr << "\tviewportSubPixelBits: " << limits.viewportSubPixelBits << endl;
        cerr << "\tminMemoryMapAlignment: " << limits.minMemoryMapAlignment << endl;

        // VkDeviceSize (uint64_t)
        cerr << "\tminTexelBufferOffsetAlignment: " << limits.minTexelBufferOffsetAlignment << endl;
        cerr << "\tminUniformBufferOffsetAlignment: " << limits.minUniformBufferOffsetAlignment << endl;
        cerr << "\tminStorageBufferOffsetAlignment: " << limits.minStorageBufferOffsetAlignment << endl;

        cerr << "\tminTexelOffset: " << limits.minTexelOffset << endl;
        cerr << "\tmaxTexelOffset: " << limits.maxTexelOffset << endl;
        cerr << "\tminTexelGatherOffset: " << limits.minTexelGatherOffset << endl;
        cerr << "\tmaxTexelGatherOffset: " << limits.maxTexelGatherOffset << endl;
        cerr << "\tminInterpolationOffset: " << limits.minInterpolationOffset << endl;
        cerr << "\tmaxInterpolationOffset: " << limits.maxInterpolationOffset << endl;
        cerr << "\tsubPixelInterpolationOffsetBits: " << limits.subPixelInterpolationOffsetBits << endl;
        cerr << "\tmaxFramebufferWidth: " << limits.maxFramebufferWidth << endl;
        cerr << "\tmaxFramebufferHeight: " << limits.maxFramebufferHeight << endl;
        cerr << "\tmaxFramebufferLayers: " << limits.maxFramebufferLayers << endl;

        // VkSampleCountFlags
        cerr << "\tframebufferColorSampleCounts: " << limits.framebufferColorSampleCounts << endl;
        cerr << "\tframebufferDepthSampleCounts: " << limits.framebufferDepthSampleCounts << endl;
        cerr << "\tframebufferStencilSampleCounts: " << limits.framebufferStencilSampleCounts << endl;
        cerr << "\tframebufferNoAttachmentsSampleCounts: " << limits.framebufferNoAttachmentsSampleCounts << endl;

        cerr << "\tmaxColorAttachments: " << limits.maxColorAttachments << endl;
        
        // VkSampleCountFlags
        cerr << "\tsampledImageColorSampleCounts: " << limits.sampledImageColorSampleCounts << endl;
        cerr << "\tsampledImageIntegerSampleCounts: " << limits.sampledImageIntegerSampleCounts << endl;
        cerr << "\tsampledImageDepthSampleCounts: " << limits.sampledImageDepthSampleCounts << endl;
        cerr << "\tsampledImageStencilSampleCounts: " << limits.sampledImageStencilSampleCounts << endl;
        cerr << "\tstorageImageSampleCounts: " << limits.storageImageSampleCounts << endl;
        cerr << "\tmaxSampleMaskWords: " << limits.maxSampleMaskWords << endl;
        cerr << "\ttimestampComputeAndGraphics: " << limits.timestampComputeAndGraphics << endl;
        cerr << "\ttimestampPeriod: " << limits.timestampPeriod << endl;
        cerr << "\tmaxClipDistances: " << limits.maxClipDistances << endl;
        cerr << "\tmaxCullDistances: " << limits.maxCullDistances << endl;
        cerr << "\tdiscreteQueuePriorities: " << limits.discreteQueuePriorities << endl;

        cerr << "\tpointSizeRange[" << _countof(limits.pointSizeRange) << "]" << endl;
        {
            uint32_t i = 0;
            for (float pointSize : limits.pointSizeRange)
                cerr << "\t\tpointSizeRange[" << i++ << "] = " << pointSize << endl;
        }

        cerr << "\tlineWidthRange[" << _countof(limits.lineWidthRange) << "]" << endl;
        {
            uint32_t i = 0;
            for (float lineWidth : limits.lineWidthRange)
                cerr << "\t\tlineWidthRange[" << i++ << "] = " << lineWidth << endl;
        }

        cerr << "\tpointSizeGranularity: " << limits.pointSizeGranularity << endl;
        cerr << "\tlineWidthGranularity: " << limits.lineWidthGranularity << endl;
        cerr << "\tstrictLines: " << limits.strictLines << endl;
        cerr << "\tstandardSampleLocations: " << limits.standardSampleLocations << endl;

        // VkDeviceSize (uint64_t)
        cerr << "\toptimalBufferCopyOffsetAlignment: " << limits.optimalBufferCopyOffsetAlignment << endl;
        cerr << "\toptimalBufferCopyRowPitchAlignment: " << limits.optimalBufferCopyRowPitchAlignment << endl;
        cerr << "\tnonCoherentAtomSize: " << limits.nonCoherentAtomSize << endl;
    }

    void logDeviceSparseProps(const VkPhysicalDeviceSparseProperties& sparseProps)
    {
        cerr << "PhysicalDevice SparseProperties" << endl;
        cerr << "\tResidency Standard 2D Block Shape: " << sparseProps.residencyStandard2DBlockShape << endl;
        cerr << "\tResidency Standard 2D Multisample Block Shape: " << sparseProps.residencyStandard2DMultisampleBlockShape << endl;
        cerr << "\tResidency Standard 3D Block Shape: " << sparseProps.residencyStandard3DBlockShape << endl;
        cerr << "\tResidency Aligned Mip Size: " << sparseProps.residencyAlignedMipSize << endl;
        cerr << "\tResidency NonResident Strict: " << sparseProps.residencyNonResidentStrict << endl;
    }

    void logDeviceFeatures(const VkPhysicalDeviceFeatures& deviceFeatures)
    {
        cerr << "PhysicalDevice Features" << endl;
        cerr << "\trobustBufferAccess: " << deviceFeatures.robustBufferAccess << endl;
        cerr << "\tfullDrawIndexUint32: " << deviceFeatures.fullDrawIndexUint32 << endl;
        cerr << "\timageCubeArray: " << deviceFeatures.imageCubeArray << endl;
        cerr << "\tindependentBlend: " << deviceFeatures.independentBlend << endl;
        cerr << "\tgeometryShader: " << deviceFeatures.geometryShader << endl;
        cerr << "\ttessellationShader: " << deviceFeatures.tessellationShader << endl;
        cerr << "\tsampleRateShading: " << deviceFeatures.sampleRateShading << endl;
        cerr << "\tdualSrcBlend: " << deviceFeatures.dualSrcBlend << endl;
        cerr << "\tlogicOp: " << deviceFeatures.logicOp << endl;
        cerr << "\tmultiDrawIndirect: " << deviceFeatures.multiDrawIndirect << endl;
        cerr << "\tdrawIndirectFirstInstance: " << deviceFeatures.drawIndirectFirstInstance << endl;
        cerr << "\tdepthClamp: " << deviceFeatures.depthClamp << endl;
        cerr << "\tdepthBiasClamp: " << deviceFeatures.depthBiasClamp << endl;
        cerr << "\tfillModeNonSolid: " << deviceFeatures.fillModeNonSolid << endl;
        cerr << "\tdepthBounds: " << deviceFeatures.depthBounds << endl;
        cerr << "\twideLines: " << deviceFeatures.wideLines << endl;
        cerr << "\tlargePoints: " << deviceFeatures.largePoints << endl;
        cerr << "\talphaToOne: " << deviceFeatures.alphaToOne << endl;
        cerr << "\tmultiViewport: " << deviceFeatures.multiViewport << endl;
        cerr << "\tsamplerAnisotropy: " << deviceFeatures.samplerAnisotropy << endl;
        cerr << "\ttextureCompressionETC2: " << deviceFeatures.textureCompressionETC2 << endl;
        cerr << "\ttextureCompressionASTC_LDR: " << deviceFeatures.textureCompressionASTC_LDR << endl;
        cerr << "\ttextureCompressionBC: " << deviceFeatures.textureCompressionBC << endl;
        cerr << "\tocclusionQueryPrecise: " << deviceFeatures.occlusionQueryPrecise << endl;
        cerr << "\tpipelineStatisticsQuery: " << deviceFeatures.pipelineStatisticsQuery << endl;
        cerr << "\tvertexPipelineStoresAndAtomics: " << deviceFeatures.vertexPipelineStoresAndAtomics << endl;
        cerr << "\tfragmentStoresAndAtomics: " << deviceFeatures.fragmentStoresAndAtomics << endl;
        cerr << "\tshaderTessellationAndGeometryPointSize: " << deviceFeatures.shaderTessellationAndGeometryPointSize << endl;
        cerr << "\tshaderImageGatherExtended: " << deviceFeatures.shaderImageGatherExtended << endl;
        cerr << "\tshaderStorageImageExtendedFormats: " << deviceFeatures.shaderStorageImageExtendedFormats << endl;
        cerr << "\tshaderStorageImageMultisample: " << deviceFeatures.shaderStorageImageMultisample << endl;
        cerr << "\tshaderStorageImageReadWithoutFormat: " << deviceFeatures.shaderStorageImageReadWithoutFormat << endl;
        cerr << "\tshaderStorageImageWriteWithoutFormat: " << deviceFeatures.shaderStorageImageWriteWithoutFormat << endl;
        cerr << "\tshaderUniformBufferArrayDynamicIndexing: " << deviceFeatures.shaderUniformBufferArrayDynamicIndexing << endl;
        cerr << "\tshaderSampledImageArrayDynamicIndexing: " << deviceFeatures.shaderSampledImageArrayDynamicIndexing << endl;
        cerr << "\tshaderStorageBufferArrayDynamicIndexing: " << deviceFeatures.shaderStorageBufferArrayDynamicIndexing << endl;
        cerr << "\tshaderStorageImageArrayDynamicIndexing: " << deviceFeatures.shaderStorageImageArrayDynamicIndexing << endl;
        cerr << "\tshaderClipDistance: " << deviceFeatures.shaderClipDistance << endl;
        cerr << "\tshaderCullDistance: " << deviceFeatures.shaderCullDistance << endl;
        cerr << "\tshaderFloat64: " << deviceFeatures.shaderFloat64 << endl;
        cerr << "\tshaderInt64: " << deviceFeatures.shaderInt64 << endl;
        cerr << "\tshaderInt16: " << deviceFeatures.shaderInt16 << endl;
        cerr << "\tshaderResourceResidency: " << deviceFeatures.shaderResourceResidency << endl;
        cerr << "\tshaderResourceMinLod: " << deviceFeatures.shaderResourceMinLod << endl;
        cerr << "\tsparseBinding: " << deviceFeatures.sparseBinding << endl;
        cerr << "\tsparseResidencyBuffer: " << deviceFeatures.sparseResidencyBuffer << endl;
        cerr << "\tsparseResidencyImage2D: " << deviceFeatures.sparseResidencyImage2D << endl;
        cerr << "\tsparseResidencyImage3D: " << deviceFeatures.sparseResidencyImage3D << endl;
        cerr << "\tsparseResidency2Samples: " << deviceFeatures.sparseResidency2Samples << endl;
        cerr << "\tsparseResidency4Samples: " << deviceFeatures.sparseResidency4Samples << endl;
        cerr << "\tsparseResidency8Samples: " << deviceFeatures.sparseResidency8Samples << endl;
        cerr << "\tsparseResidency16Samples: " << deviceFeatures.sparseResidency16Samples << endl;
        cerr << "\tsparseResidencyAliased: " << deviceFeatures.sparseResidencyAliased << endl;
        cerr << "\tvariableMultisampleRate: " << deviceFeatures.variableMultisampleRate << endl;
        cerr << "\tinheritedQueries: " << deviceFeatures.inheritedQueries << endl;
    }

    bool isDeviceSuitable(const VkPhysicalDevice& device, const LogProfile& logProfile)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        if (logProfile.logProps) {
            logDeviceProps(deviceProperties);
        }
        
        if (logProfile.logLimits) {
            logDeviceLimits(deviceProperties.limits);
        }

        if (logProfile.logSparseProps) {
            logDeviceSparseProps(deviceProperties.sparseProperties);
        }

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        if (logProfile.logFeatures) {
            logDeviceFeatures(deviceFeatures);
        }

        return true;
    }

    void logDeviceQueueFamily(const string& name, const VkQueueFamilyProperties& queueFamily)
    {
        cerr << "PhysicalDevice Queue Family " << name << endl;
        cerr << "\tqueueFlags: " << queueFamily.queueFlags << endl;
        cerr << "\tqueueCount: " << queueFamily.queueCount << endl;
        cerr << "\ttimestampValidBits: " << queueFamily.timestampValidBits << endl;
        cerr << "\tminImageTransferGranularity:" << endl;
        cerr << "\t\twidth: " << queueFamily.minImageTransferGranularity.width << endl;
        cerr << "\t\theight: " << queueFamily.minImageTransferGranularity.height << endl;
        cerr << "\t\tdepth: " << queueFamily.minImageTransferGranularity.depth << endl;
    }

    QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& device, const LogProfile& logProfile)
    {
        QueueFamilyIndices queueIndices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                queueIndices.graphicsFamily = i;
                if (logProfile.loqGraphicsQueue)
                    logDeviceQueueFamily("Graphics", queueFamily);
            }
            else if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                queueIndices.computeFamily = i;
                if (logProfile.logComputeQueue)
                    logDeviceQueueFamily("Compute", queueFamily);
            }
            else if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
                queueIndices.xferFamily = i;
                if (logProfile.logXferQueue)
                    logDeviceQueueFamily("Transfer", queueFamily);
            }

            ++i;
        }

        return queueIndices;
    }

    uint32_t rateDeviceSuitability(const VkPhysicalDevice& device, const LogProfile& logProfile)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        if (logProfile.logProps) {
            logDeviceProps(deviceProperties);
        }

        if (logProfile.logLimits) {
            logDeviceLimits(deviceProperties.limits);
        }

        if (logProfile.logSparseProps) {
            logDeviceSparseProps(deviceProperties.sparseProperties);
        }

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        if (logProfile.logFeatures) {
            logDeviceFeatures(deviceFeatures);
        }
        
        QueueFamilyIndices queueIndices = findQueueFamilies(device, logProfile);

        // need geometry shader
        if (!deviceFeatures.geometryShader)
            return 0;

        // need graphics queue
        if (!queueIndices.HasGraphicsQueue())
            return 0;

        // need compute queue
        if (!queueIndices.HasComputeQueue())
            return 0;

        uint32_t score = 0;

        // descrete GPUs are preferred
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            score += 1000;

        // maximum texture size affects image quality
        score += deviceProperties.limits.maxImageDimension2D;

        return score;
    }

    void pickPhysicalDevice()
    {
        LogProfile logProfile;

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0)
            throw runtime_error("failed to find GPUs with Vulkan support!");

        vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        multimap<uint32_t, VkPhysicalDevice> candidates;
        for (const auto& device : devices)
        {
            uint32_t score = rateDeviceSuitability(device, logProfile);
            candidates.insert(std::make_pair(score, device));
        }

        // find the best candidate
        if (candidates.rbegin()->first > 0)
            physicalDevice = candidates.rbegin()->second;

        if (physicalDevice == VK_NULL_HANDLE)
            throw runtime_error("failed to find suitable GPU");
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback
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

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
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

    void setupDebugMessenger()
    {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
        {
            throw runtime_error("failed to set up debug messenger");
        }
    }

    bool checkValidationLayerSupport()
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

    vector<const char*> getRequiredExtensions()
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

    void createInstance()
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

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        {
            throw runtime_error("failed to create instance");
        }
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(pWindow))
        {
            glfwPollEvents();
        }
    }

    void cleanUp()
    {
        if (enableValidationLayers)
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);

        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(pWindow);
        glfwTerminate();
    }
};

int main()
{
    std::cout.setf(std::ios::boolalpha);
    std::cerr.setf(std::ios::boolalpha);

    HelloTriangleApp app;

    try
    {
        app.run();
    }
    catch (const exception& e)
    {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
