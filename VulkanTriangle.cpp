#include "VulkanTriangle.h"
#include "Utils.h"

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

//#define USE_SHADER_OBJ

// https://github.com/KhronosGroup/Vulkan-ValidationLayers/pull/5570
#ifdef USE_SHADER_OBJ
const char* VK_EXT_SHADER_OBJ = "VK_EXT_shader_object";
const char* VK_EXT_EXTENDED_DYNAMIC_STATE = "VK_EXT_extended_dynamic_state";
const char* VK_EXT_EXTENDED_DYNAMIC_STATE2 = "VK_EXT_extended_dynamic_state2";
const char* VK_EXT_EXTENDED_DYNAMIC_STATE3 = "VK_EXT_extended_dynamic_state3";
const char* VK_EXT_VERTEX_INPUT_DYNAMIC_STATE = "VK_EXT_vertex_input_dynamic_state";
#endif

const vector<const char*> deviceExtensions =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
#if defined WIN32 && defined USE_SHADER_OBJ
    , VK_EXT_SHADER_OBJ
    , VK_EXT_EXTENDED_DYNAMIC_STATE
    , VK_EXT_EXTENDED_DYNAMIC_STATE2
    , VK_EXT_EXTENDED_DYNAMIC_STATE3
    , VK_EXT_VERTEX_INPUT_DYNAMIC_STATE
#endif
};


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
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
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
    for (auto pFramebuffer : swapChainFramebuffers)
        vkDestroyFramebuffer(pDevice, pFramebuffer, nullptr);

    vkDestroyPipeline(pDevice, pGraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(pDevice, pPipelineLayout, nullptr);
    vkDestroyRenderPass(pDevice, pRenderPass, nullptr);

    for (auto imageView : swapChainImageViews)
        vkDestroyImageView(pDevice, imageView, nullptr);

    vkDestroySwapchainKHR(pDevice, pSwapChain, nullptr);
    vkDestroyDevice(pDevice, nullptr);

    if (enableValidationLayers)
        Utils::DestroyDebugUtilsMessengerEXT(pInstance, pDebugMessenger, nullptr);

    vkDestroySurfaceKHR(pInstance, pSurface, nullptr);
    vkDestroyInstance(pInstance, nullptr);

    glfwDestroyWindow(pWindow);
    glfwTerminate();
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


void VulkanTriangleApp::setupDebugMessenger()
{
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);

    if (Utils::CreateDebugUtilsMessengerEXT(pInstance, &createInfo, nullptr, &pDebugMessenger) != VK_SUCCESS)
    {
        throw runtime_error("failed to set up debug messenger");
    }
}


void VulkanTriangleApp::createSurface()
{
    if (glfwCreateWindowSurface(pInstance, pWindow, nullptr, &pSurface) != VK_SUCCESS)
        throw runtime_error("failed to create window surface");
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


void VulkanTriangleApp::createImageViews()
{
    // resize list to match swapchain images
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); ++i)
    {
        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = swapChainImages[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = swapChainImageFormat;

        // image swizzling (red, blue, green, alpha) channels
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // image purpose and which part should be accessed
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;      // more for stereoscopic 3D

        if (vkCreateImageView(pDevice, &imageViewCreateInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
            throw runtime_error("failed to create image views");
    }
}


void VulkanTriangleApp::createRenderPass()
{
    // loadOp and storeOp determine what to do with the data in the attachment before rendering and after rendering
    // loadOp  : VK_ATTACHMENT_LOAD_OP_LOAD       - preserve existing contents of the attachment
    //         : VK_ATTACHMENT_LOAD_OP_CLEAR      - clear to constant
    //         : VK_ATTACHMENT_LOAD_OP_DONT_CARE  - existing contents are undefined
    // 
    // storeOp : VK_ATTACHMENT_STORE_OP_STORE     - rendered contents will be store in memory and can be read later
    //         : VK_ATTACHMENT_STORE_OP_DONT_CARE - undefined
    //
    // Textures and Framebuffers in Vulkan are represented by VkImage objects with specific pixel formats.
    // The layout of the pixels in memory can change based on what you are trying to do with an image.
    // 
    //         : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL - images used as color attachment
    //         : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR          - images to be presented to swap chain
    //         : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL     - images to be used as destination for a memory copy
    // 
    // initialLayout : specifies which layout the image will have before the render pass begins
    // finalLayout   : specifies the layout to automatically transition to when the render pass finishes
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // attachment - index of attachment
    //            - layout(location = 0) out vec4 outColor
    // 
    // layout     - subpass transition optimal is the best option for performance
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // pDepthStencilAttachment - attachment for depthStencil
    // pInputAttachments       - attachments that are read from a shader
    // pResolveAttachments     - attachments used for multisampling color attachments
    // pPreserveAttachments    - attachments that are not used by this subpass but the data must be preserved
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassCreateInfo{};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &colorAttachment;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(pDevice, &renderPassCreateInfo, nullptr, &pRenderPass) != VK_SUCCESS)
        throw runtime_error("failed to create render pass");
}


void VulkanTriangleApp::createGraphicsPipeline()
{
    const char* ndcVertShaderFilename = "shaders/ndcVert.spv";
    const char* ndcFragShaderFilename = "shaders/ndcFrag.spv";
    const char* vertexColorVertFilename = "shaders/vertexColorVert.spv";
    const char* vertexColorFragFilename = "shaders/vertexColorFrag.spv";

    auto vertShaderCode = Utils::readFile(ndcVertShaderFilename);
    auto fragShaderCode = Utils::readFile(ndcFragShaderFilename);

    // compilation from byteCode to machineCode for execution on the GPU does not happen until it is created in the pipeline
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    // assign shader to specific pipeline stage (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | ...)
    VkPipelineShaderStageCreateInfo vertPipelineShaderStageInfo{};
    vertPipelineShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertPipelineShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertPipelineShaderStageInfo.module = vertShaderModule;
    vertPipelineShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragPipelineShaderStageInfo{};
    fragPipelineShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragPipelineShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragPipelineShaderStageInfo.module = fragShaderModule;
    fragPipelineShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertPipelineShaderStageInfo, fragPipelineShaderStageInfo };

    vector<VkVertexInputBindingDescription> vertexInputBindings;
    vector<VkVertexInputAttributeDescription> vertexInputAttrDescriptions;

    // fixed functions - { DynamicState, VertexInput, InputAssembly, ViewportScissors, Rasterizer, Multisampling, D24S8, ColorBlending, PipelineLayout

    // DynamicState - viewport size, ling width, blend constants
    // keep these out of the pipeline state and specify these at draw time
    vector<VkDynamicState> dynamicStates =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

    // VertexInput
    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
    vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
    vertexInputStateCreateInfo.pVertexBindingDescriptions = vertexInputBindings.data();
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttrDescriptions.size());
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttrDescriptions.data();

    // Input Assembly
    // can restart topology with indices 0xFFFF or 0xFFFFFFFF
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    // Viewports and Scissor Rects
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;

    // viewport and scissors are set dynmically
    // but count needs to be set (if I understood correctly)
    VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.scissorCount = 1;

    // Rasterizer
    // depthClampEnable keeps fragmenst outside of near/far planes
    // rasterizerDiscardEnable does not allow geometry pass through the rasterizer stage
    // polgonMode : VK_POLYGON_MODE_FILL | VK_POLYGON_MODE_LINE | VK_POLYGON_MODE_POINT | VK_POLOYGON_FILL_RECTANGLE_NV
    // lineWidth greater than 1.0 needs to enable GPU feature wideLines
    // cullMode : VK_CULL_MODE_BACK_BIT | VK_CULL_MODE_FRONT_BIT | VK_CULL_MODE_NONE | VK_CULL_MODE_FRONT_AND_BACK
    // frontFace : VK_FRONT_FACE_CLOCKWISE | VK_FRONT_FACE_COUNTER_CLOCKWISE
    VkPipelineRasterizationStateCreateInfo rasterizerStateCreateInfo{};
    rasterizerStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerStateCreateInfo.depthClampEnable = VK_FALSE;
    rasterizerStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizerStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizerStateCreateInfo.lineWidth = 1.0f;
    rasterizerStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizerStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizerStateCreateInfo.depthBiasEnable = VK_FALSE;
    rasterizerStateCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizerStateCreateInfo.depthBiasClamp = 0.0f;
    rasterizerStateCreateInfo.depthBiasSlopeFactor = 0.0f;

    // Multi-sampling
    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
    multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleStateCreateInfo.minSampleShading = 1.0f;
    multisampleStateCreateInfo.pSampleMask = nullptr;
    multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

    // Depth and Stencil Testing

    // Color Blending
    VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
    enableAlphaBlending(colorBlendAttachmentState);

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
    colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendStateCreateInfo.attachmentCount = 1;
    colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;
    colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
    colorBlendStateCreateInfo.blendConstants[3] = 0.0f;

    // PipelineLayout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = nullptr;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(pDevice, &pipelineLayoutCreateInfo, nullptr, &pPipelineLayout) != VK_SUCCESS)
        throw runtime_error("failed to create pipeline layout");

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
    graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCreateInfo.stageCount = 2;
    graphicsPipelineCreateInfo.pStages = shaderStages;
    graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    graphicsPipelineCreateInfo.pRasterizationState = &rasterizerStateCreateInfo;
    graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
    graphicsPipelineCreateInfo.pDepthStencilState = nullptr;
    graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
    graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    graphicsPipelineCreateInfo.layout = pPipelineLayout;
    graphicsPipelineCreateInfo.renderPass = pRenderPass;
    graphicsPipelineCreateInfo.subpass = 0;
    graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    graphicsPipelineCreateInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(pDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &pGraphicsPipeline) != VK_SUCCESS)
        throw runtime_error("failed to create graphics pipeline");

    // cleanup shader modules
    vkDestroyShaderModule(pDevice, vertShaderModule, nullptr);
    vkDestroyShaderModule(pDevice, fragShaderModule, nullptr);
}


void VulkanTriangleApp::createFramebuffers()
{
    // create a framebuffer for each imageView in the swapChain
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); ++i)
    {
        // a temporary to allow for colorBuffer, depthBuffer, etc. ?
        // attachmentCount needs to match array size
        // layers stereoscopic rendering?
        VkImageView attachments[] =
        {
            swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferCreateInfo{};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = pRenderPass;
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.pAttachments = attachments;
        framebufferCreateInfo.width = swapChainExtent.width;
        framebufferCreateInfo.height = swapChainExtent.height;
        framebufferCreateInfo.layers = 1;

        if (vkCreateFramebuffer(pDevice, &framebufferCreateInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
            throw runtime_error("failed to create framebuffer");
    }

}


void VulkanTriangleApp::createCommandPool()
{
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

    // geometry shader lacks performance (VkPhysicalDeviceFeatures::geometryShader)
    // need tessellationShader
    if (!deviceFeatures.tessellationShader)
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

    if (logProfile.logExtensions)
        Logging::logDeviceExtensions(availableExtensions);

    set<string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
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


VkShaderModule VulkanTriangleApp::createShaderModule(const vector<unsigned char>& shaderCode)
{
    VkShaderModuleCreateInfo shaderModuleCreateInfo{};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = shaderCode.size();
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(pDevice, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw runtime_error("failed to create shader module");
    }

    return shaderModule;
}


void VulkanTriangleApp::enableAlphaBlending(VkPipelineColorBlendAttachmentState& colorBlendAttachmentState)
{
    colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachmentState.blendEnable = VK_TRUE;
    colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
}


void VulkanTriangleApp::disableAlphaBlending(VkPipelineColorBlendAttachmentState& colorBlendAttachmentState)
{
    colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachmentState.blendEnable = VK_FALSE;
    colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
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
