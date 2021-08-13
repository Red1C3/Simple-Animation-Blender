#include <simple-animation-blender/Application.h>
using namespace std;
using namespace glm;
Application::Application()
{
}
Application &Application::instance()
{
    static Application app;
    return app;
}
void Application::init(char *meshPath)
{
    this->meshPath = meshPath;
    createWindow(720, 1280);
    createVkInstance();
    createPhysicalDevice();
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
    createSurface();
    createQueuesFamilies();
    createLogicalDevice();
    swapchain = createSwapchain();
    createCommandPool();
    createDescriptorPool();
    createRenderPass();
    createFramebuffers();
    createDescriptorSetLayout();
    createPipelineLayout();
    createPipeline();
    mesh = new Mesh(meshPath);
    createVertexBuffer();
}
void Application::createWindow(int height, int width)
{

    if (glfwInit() != GLFW_TRUE)
    {
        ERR("Failed to init GLFW");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(width, height, "Simple-Animation-Blender", nullptr, nullptr);
    if (window == nullptr)
        ERR("Failed to create window");
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    LOG("Created window successfully");
}
void Application::createVkInstance()
{
    VkApplicationInfo appInfo{};
    appInfo.apiVersion = VK_API_VERSION_1_2;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    appInfo.pApplicationName = "Simple Animation Blender";
    appInfo.pEngineName = "Simple Animation Blender";
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    VkInstanceCreateInfo createInfo{};
    uint32_t glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    createInfo.enabledExtensionCount = glfwExtensionCount;
    const char *validationLayer = "VK_LAYER_KHRONOS_validation";
    if (debug)
    {
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = &validationLayer;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }
    createInfo.pApplicationInfo = &appInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    if (vkCreateInstance(&createInfo, ALLOCATOR, &vkInstance) != VK_SUCCESS)
    {
        ERR("Failed to create Vulkan instance");
    }
    LOG("Vulkan instance created successfully");
}
void Application::createPhysicalDevice()
{
    uint32_t physicalDevicesCount;
    vkEnumeratePhysicalDevices(vkInstance, &physicalDevicesCount, nullptr);
    vector<VkPhysicalDevice> physicalDevices(physicalDevicesCount);
    vkEnumeratePhysicalDevices(vkInstance, &physicalDevicesCount, physicalDevices.data());
    VkPhysicalDevice integratedFallback = VK_NULL_HANDLE;
    for (VkPhysicalDevice device : physicalDevices)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            physicalDevice = device;
            return;
        }
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        {
            integratedFallback = device;
        }
    }
    if (integratedFallback == VK_NULL_HANDLE)
    {
        ERR("No suitable GPU was found");
    }
    LOG("Warning: using integrated GPU becuz no discrete was found");
    physicalDevice = integratedFallback;
}
void Application::createQueuesFamilies()
{
    uint32_t queueFamiliesCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, nullptr);
    vector<VkQueueFamilyProperties> queueFamiliesProperties(queueFamiliesCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, queueFamiliesProperties.data());
    for (uint32_t i = 0; i < queueFamiliesProperties.size(); ++i)
    {
        bool isGraphicsQueue = false;
        VkBool32 isPresentQueue = VK_FALSE;
        if (queueFamiliesProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            isGraphicsQueue = true;
        }
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &isPresentQueue);
        if (isPresentQueue && isGraphicsQueue)
        {
            graphicsQueueFamilyIndex = i;
            presentQueueFamilyIndex = i;
            graphicsQueuesCount = queueFamiliesProperties[i].queueCount;
            presentQueuesCount = queueFamiliesProperties[i].queueCount;
            sameQueueForGraphicsAndPresent = true;
            return;
        }
    }
    for (uint32_t i = 0; i < queueFamiliesProperties.size(); ++i)
    {
        if (queueFamiliesProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphicsQueueFamilyIndex = i;
            graphicsQueuesCount = queueFamiliesProperties[i].queueCount;
        }
        VkBool32 isPresentQueue = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &isPresentQueue);
        if (isPresentQueue)
        {
            presentQueueFamilyIndex = i;
            presentQueuesCount = queueFamiliesProperties[i].queueCount;
        }
    }
}
void Application::createSurface()
{
    if (glfwCreateWindowSurface(vkInstance, window, ALLOCATOR, &surface) != VK_SUCCESS)
    {
        ERR("Failed to obtain GLFW surface");
    }
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities) != VK_SUCCESS)
    {
        ERR("Failed to get surface capabilities");
    }
    uint32_t formatsCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, nullptr);
    vector<VkSurfaceFormatKHR> formats(formatsCount);
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, formats.data()) != VK_SUCCESS)
    {
        ERR("Failed to get surface formats");
    }
    surfaceFormat = formats[0];
}
void Application::createLogicalDevice()
{
    vector<VkDeviceQueueCreateInfo> queuesCreateInfo;
    if (sameQueueForGraphicsAndPresent)
    {
        queuesCreateInfo.resize(1);
    }
    else
    {
        queuesCreateInfo.resize(2);
    }
    vector<float> graphicsPriorities, presentPriorities;
    for (uint32_t i = 0; i < queuesCreateInfo.size(); ++i)
    {
        queuesCreateInfo[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        if (i == 0)
        {
            queuesCreateInfo[i].queueFamilyIndex = graphicsQueueFamilyIndex;
            queuesCreateInfo[i].queueCount = graphicsQueuesCount;
            graphicsPriorities.resize(graphicsQueuesCount);
            queuesCreateInfo[i].pQueuePriorities = graphicsPriorities.data();
        }
        else
        {
            queuesCreateInfo[i].queueFamilyIndex = presentQueueFamilyIndex;
            queuesCreateInfo[i].queueCount = presentQueuesCount;
            presentPriorities.resize(presentQueuesCount);
            queuesCreateInfo[i].pQueuePriorities = presentPriorities.data();
        }
    }
    VkDeviceCreateInfo createInfo{};
    createInfo.enabledExtensionCount = 1;
    createInfo.enabledLayerCount = 0;
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    const char *swapchainExt = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    createInfo.ppEnabledExtensionNames = &swapchainExt;
    createInfo.queueCreateInfoCount = queuesCreateInfo.size();
    createInfo.pQueueCreateInfos = queuesCreateInfo.data();
    if (vkCreateDevice(physicalDevice, &createInfo, ALLOCATOR, &device) != VK_SUCCESS)
    {
        ERR("Failed to create logical device");
    }
    LOG("Created logical device successfully");
    vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
}
VkSwapchainKHR Application::createSwapchain(VkSwapchainKHR oldSwapchain)
{
    VkSwapchainKHR swapchain;
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = surfaceCapabilities.minImageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = surfaceCapabilities.currentExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = surfaceCapabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    createInfo.clipped = VK_FALSE;
    createInfo.oldSwapchain = oldSwapchain;
    if (vkCreateSwapchainKHR(device, &createInfo, ALLOCATOR, &swapchain) != VK_SUCCESS)
    {
        ERR("Failed to create/recreate swapchain");
    }
    if (oldSwapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(device, oldSwapchain, ALLOCATOR);
    }
    return swapchain;
}
void Application::createCommandPool()
{
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(device, &createInfo, ALLOCATOR, &cmdPool) != VK_SUCCESS)
    {
        ERR("Failed to create command pool");
    }
}
void Application::createDescriptorPool()
{
    VkDescriptorPoolCreateInfo createInfo{};
    VkDescriptorPoolSize poolSizes[] =
        {
            {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    createInfo.maxSets = 1000 * 11;
    createInfo.poolSizeCount = 11;
    createInfo.pPoolSizes = poolSizes;
    if (vkCreateDescriptorPool(device, &createInfo, ALLOCATOR, &descriptorPool) != VK_SUCCESS)
    {
        ERR("Failed to create descriptor pool");
    }
}
void Application::createRenderPass()
{
    VkAttachmentDescription attachmentDescriptions[2];
    attachmentDescriptions[0].flags = 0;
    attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachmentDescriptions[0].format = surfaceFormat.format;
    attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescriptions[1].flags = 0;
    attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachmentDescriptions[1].format = VK_FORMAT_D24_UNORM_S8_UINT;
    attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkAttachmentReference attachmentReferences[2];
    attachmentReferences[0].attachment = 0;
    attachmentReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentReferences[1].attachment = 1;
    attachmentReferences[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subPassDescription{};
    subPassDescription.colorAttachmentCount = 1;
    subPassDescription.inputAttachmentCount = 0;
    subPassDescription.pColorAttachments = &attachmentReferences[0];
    subPassDescription.pDepthStencilAttachment = &attachmentReferences[1];
    subPassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subPassDescription.preserveAttachmentCount = 0;
    subPassDescription.pResolveAttachments = nullptr;
    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = 2;
    createInfo.pAttachments = attachmentDescriptions;
    createInfo.dependencyCount = 0;
    createInfo.pSubpasses = &subPassDescription;
    createInfo.subpassCount = 1;
    if (vkCreateRenderPass(device, &createInfo, ALLOCATOR, &renderPass) != VK_SUCCESS)
    {
        ERR("Failed to create render pass");
    }
}
void Application::createFramebuffers()
{
    uint32_t swapchainImagesCount;
    vkGetSwapchainImagesKHR(device, swapchain, &swapchainImagesCount, nullptr);
    vector<VkImage> swapchainImages(swapchainImagesCount);
    if (vkGetSwapchainImagesKHR(device, swapchain, &swapchainImagesCount, swapchainImages.data()) != VK_SUCCESS)
    {
        ERR("Failed to get swapchain images");
    }
    framebuffers.resize(swapchainImagesCount);
    VkImageCreateInfo depthImgCreateInfo{};
    depthImgCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depthImgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    depthImgCreateInfo.format = VK_FORMAT_D24_UNORM_S8_UINT;
    depthImgCreateInfo.extent.depth = 1;
    depthImgCreateInfo.extent.height = fbHeight;
    depthImgCreateInfo.extent.width = fbWidth;
    depthImgCreateInfo.mipLevels = 1;
    depthImgCreateInfo.arrayLayers = 1;
    depthImgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    depthImgCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    depthImgCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depthImgCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    depthImgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    for (uint32_t i = 0; i < framebuffers.size(); ++i)
    {
        if (vkCreateImage(device, &depthImgCreateInfo, ALLOCATOR, &(framebuffers[i].depthImg)) != VK_SUCCESS)
        {
            ERR(FORMAT("Failed to create depth img for {} framebuffer", i));
        }
        VkMemoryRequirements memReq;
        vkGetImageMemoryRequirements(device, framebuffers[i].depthImg, &memReq);
        framebuffers[i].depthImgMemory = allocateMemory(memReq, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (vkBindImageMemory(device, framebuffers[i].depthImg, framebuffers[i].depthImgMemory, 0) != VK_SUCCESS)
        {
            ERR(FORMAT("Failed to bind depth image with allocated memory for {} framebuffer", i));
        }
        VkImageViewCreateInfo depthImgViewCI{};
        depthImgViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        depthImgViewCI.image = framebuffers[i].depthImg;
        depthImgViewCI.components = {VK_COMPONENT_SWIZZLE_IDENTITY,
                                     VK_COMPONENT_SWIZZLE_IDENTITY,
                                     VK_COMPONENT_SWIZZLE_IDENTITY,
                                     VK_COMPONENT_SWIZZLE_IDENTITY};
        depthImgViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        depthImgViewCI.format = VK_FORMAT_D24_UNORM_S8_UINT;
        depthImgViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthImgViewCI.subresourceRange.baseArrayLayer = 0;
        depthImgViewCI.subresourceRange.baseMipLevel = 0;
        depthImgViewCI.subresourceRange.layerCount = 1;
        depthImgViewCI.subresourceRange.levelCount = 1;
        if (vkCreateImageView(device, &depthImgViewCI, ALLOCATOR, &(framebuffers[i].depthImgView)) != VK_SUCCESS)
        {
            ERR(FORMAT("Failed to create depth image view for {} framebuffer", i));
        }
        VkImageViewCreateInfo swapchainImgViewCI{};
        swapchainImgViewCI.components = {VK_COMPONENT_SWIZZLE_IDENTITY,
                                         VK_COMPONENT_SWIZZLE_IDENTITY,
                                         VK_COMPONENT_SWIZZLE_IDENTITY,
                                         VK_COMPONENT_SWIZZLE_IDENTITY};
        swapchainImgViewCI.format = surfaceFormat.format;
        swapchainImgViewCI.image = swapchainImages[i];
        swapchainImgViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        swapchainImgViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        swapchainImgViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        swapchainImgViewCI.subresourceRange.baseArrayLayer = 0;
        swapchainImgViewCI.subresourceRange.baseMipLevel = 0;
        swapchainImgViewCI.subresourceRange.layerCount = 1;
        swapchainImgViewCI.subresourceRange.levelCount = 1;
        if (vkCreateImageView(device, &swapchainImgViewCI, ALLOCATOR, &(framebuffers[i].swapchainImgView)) != VK_SUCCESS)
        {
            ERR(FORMAT("Failed to create swapchain img view for {} framebuffer", i));
        }
        VkImageView attachments[2] = {framebuffers[i].swapchainImgView, framebuffers[i].depthImgView};
        VkFramebufferCreateInfo createInfo{};
        createInfo.attachmentCount = 2;
        createInfo.height = fbHeight;
        createInfo.layers = 1;
        createInfo.pAttachments = attachments;
        createInfo.renderPass = renderPass;
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.width = fbWidth;
        if (vkCreateFramebuffer(device, &createInfo, ALLOCATOR, &(framebuffers[i].vkHandle)) != VK_SUCCESS)
        {
            ERR(FORMAT("Failed to created {}th framebuffer", i));
        }
    }
}
void Application::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorCount = 1;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = 1;
    createInfo.pBindings = &binding;
    if (vkCreateDescriptorSetLayout(device, &createInfo, ALLOCATOR, &dsl) != VK_SUCCESS)
    {
        ERR("Failed to create descriptor set layout");
    }
}
void Application::createPipelineLayout()
{
    VkPipelineLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.setLayoutCount = 1;
    createInfo.pSetLayouts = &dsl;
    createInfo.pushConstantRangeCount = 0;
    if (vkCreatePipelineLayout(device, &createInfo, ALLOCATOR, &pipelineLayout) != VK_SUCCESS)
    {
        ERR("Failed to create pipeline layout");
    }
}
void Application::createPipeline()
{
    vector<char> vsCode = readBin("./Assets/vertexShader.spv");
    vector<char> fsCode = readBin("./Assets/fragmentShader.spv");
    VkShaderModule shaderModules[2];
    VkShaderModuleCreateInfo shaderModuleCI{};
    shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCI.codeSize = vsCode.size();
    shaderModuleCI.pCode = (uint32_t *)vsCode.data();
    if (vkCreateShaderModule(device, &shaderModuleCI, ALLOCATOR, &shaderModules[0]) != VK_SUCCESS)
    {
        ERR("Failed to create vertex shader modules");
    }
    shaderModuleCI.codeSize = fsCode.size();
    shaderModuleCI.pCode = (uint32_t *)fsCode.data();
    if (vkCreateShaderModule(device, &shaderModuleCI, ALLOCATOR, &shaderModules[1]) != VK_SUCCESS)
    {
        ERR("Failed to create fragment shader modules");
    }
    VkPipelineShaderStageCreateInfo shaderStages[2];
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].flags = 0;
    shaderStages[0].pNext = 0;
    shaderStages[0].pName = "main";
    shaderStages[0].pSpecializationInfo = 0;
    shaderStages[1] = shaderStages[0];
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = shaderModules[0];
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = shaderModules[1];
    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    bindingDesc.stride = 3 * sizeof(vec3) + sizeof(ivec3);
    VkVertexInputAttributeDescription attrDesc[4];
    attrDesc[0].binding = 0;
    attrDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrDesc[0].location = 0;
    attrDesc[0].offset = 0;
    attrDesc[1].binding = 0;
    attrDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrDesc[1].location = 1;
    attrDesc[1].offset = sizeof(vec3);
    attrDesc[2].binding = 0;
    attrDesc[2].format = VK_FORMAT_R32G32B32_SINT;
    attrDesc[2].location = 2;
    attrDesc[2].offset = 2 * sizeof(vec3);
    attrDesc[3].binding = 0;
    attrDesc[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrDesc[3].location = 3;
    attrDesc[3].offset = 2 * sizeof(vec3) + sizeof(ivec3);
    VkPipelineVertexInputStateCreateInfo inputState{};
    inputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    inputState.vertexBindingDescriptionCount = 1;
    inputState.pVertexBindingDescriptions = &bindingDesc;
    inputState.vertexAttributeDescriptionCount = 4;
    inputState.pVertexAttributeDescriptions = attrDesc;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    VkViewport viewport{};
    viewport.height = fbHeight;
    viewport.width = fbWidth;
    viewport.x = 0;
    viewport.y = 0;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent.width = fbWidth;
    scissor.extent.height = fbHeight;
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.pScissors = &scissor;
    VkPipelineRasterizationStateCreateInfo rasterize{};
    rasterize.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterize.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterize.rasterizerDiscardEnable = VK_FALSE;
    rasterize.polygonMode = VK_POLYGON_MODE_FILL;
    rasterize.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterize.depthBiasEnable = VK_FALSE;
    rasterize.lineWidth = 1.0f;
    VkPipelineMultisampleStateCreateInfo multiSampleState{};
    multiSampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multiSampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo depthState{};
    depthState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthState.depthTestEnable = VK_TRUE;
    depthState.depthWriteEnable = VK_TRUE;
    depthState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthState.depthBoundsTestEnable = VK_FALSE;
    depthState.stencilTestEnable = VK_FALSE;
    VkPipelineColorBlendAttachmentState attachmentState{};
    attachmentState.blendEnable = VK_FALSE;
    attachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo colorBlendState{};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.logicOpEnable = VK_FALSE;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &attachmentState;
    VkGraphicsPipelineCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    createInfo.stageCount = 2;
    createInfo.pStages = shaderStages;
    createInfo.pVertexInputState = &inputState;
    createInfo.pInputAssemblyState = &inputAssembly;
    createInfo.pViewportState = &viewportState;
    createInfo.pRasterizationState = &rasterize;
    createInfo.pMultisampleState = &multiSampleState;
    createInfo.pDepthStencilState = &depthState;
    createInfo.pColorBlendState = &colorBlendState;
    createInfo.pDynamicState = VK_NULL_HANDLE;
    createInfo.layout = pipelineLayout;
    createInfo.renderPass = renderPass;
    createInfo.subpass = 0;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &createInfo, ALLOCATOR, &pipeline) != VK_SUCCESS)
    {
        ERR("Failed to create pipeline");
    }
    vkDestroyShaderModule(device, shaderModules[0], ALLOCATOR);
    vkDestroyShaderModule(device, shaderModules[1], ALLOCATOR);
}
void Application::createVertexBuffer()
{
    VkBufferCreateInfo stagingBufferCI{};
    stagingBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferCI.size = mesh->vertices.size() * sizeof(Mesh::Vertex);
    stagingBufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkBuffer stagingBuffer;
    if (vkCreateBuffer(device, &stagingBufferCI, ALLOCATOR, &stagingBuffer) != VK_SUCCESS)
    {
        ERR("Failed to create staging vertex buffer");
    }
    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memReq);
    VkDeviceMemory stagingBufferMem = allocateMemory(memReq, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    if (vkBindBufferMemory(device, stagingBuffer, stagingBufferMem, 0) != VK_SUCCESS)
    {
        ERR("Failed to bind memory with staging vertex buffer");
    }
    void *data;
    if (vkMapMemory(device, stagingBufferMem, 0, VK_WHOLE_SIZE, 0, &data) != VK_SUCCESS)
    {
        ERR("Failed to map vertex staging buffer");
    }
    memcpy(data, mesh->vertices.data(), mesh->vertices.size() * sizeof(Mesh::Vertex));
    vkUnmapMemory(device, stagingBufferMem);
    VkBufferCreateInfo vertexBufferCI = stagingBufferCI;
    vertexBufferCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (vkCreateBuffer(device, &vertexBufferCI, ALLOCATOR, &(mesh->vertexBuffer)) != VK_SUCCESS)
    {
        ERR("Failed to create vertex buffer");
    }
    vkGetBufferMemoryRequirements(device, mesh->vertexBuffer, &memReq);
    mesh->vertexBufferMem = allocateMemory(memReq, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkBindBufferMemory(device, mesh->vertexBuffer, mesh->vertexBufferMem, 0) != VK_SUCCESS)
    {
        ERR("Failed to bind vertex buffer memory");
    }
    VkCommandBufferAllocateInfo cmdBufferAI{};
    cmdBufferAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufferAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferAI.commandPool = cmdPool;
    cmdBufferAI.commandBufferCount = 1;
    VkCommandBuffer copyBuffer;
    if (vkAllocateCommandBuffers(device, &cmdBufferAI, &copyBuffer) != VK_SUCCESS)
    {
        ERR("Failed to allocate copy cmd buffer");
    }
    VkCommandBufferBeginInfo cmdBufferBI{};
    cmdBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufferBI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(copyBuffer, &cmdBufferBI) != VK_SUCCESS)
    {
        ERR("Failed to begin copy cmd buffer");
    }
    VkBufferCopy copyRegion{};
    copyRegion.size = mesh->vertices.size() * sizeof(Mesh::Vertex);
    vkCmdCopyBuffer(copyBuffer, stagingBuffer, mesh->vertexBuffer, 1, &copyRegion);
    if (vkEndCommandBuffer(copyBuffer) != VK_SUCCESS)
    {
        ERR("Failed to end copy cmd buffer");
    }
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &copyBuffer;
    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        ERR("Failed to submit vertex buffers copying");
    }
    if (vkQueueWaitIdle(graphicsQueue) != VK_SUCCESS)
    {
        ERR("Failed to wait for graphics queue to finish");
    }
    vkDestroyBuffer(device, stagingBuffer, ALLOCATOR);
    vkFreeMemory(device, stagingBufferMem, ALLOCATOR);
    vkFreeCommandBuffers(device, cmdPool, 1, &copyBuffer);
}
VkDeviceMemory Application::allocateMemory(VkMemoryRequirements memReq, VkMemoryPropertyFlags properties)
{
    uint32_t memoryIndex;
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        const uint32_t memoryBits = (1 << i);
        const bool isRequiredMemType = memReq.memoryTypeBits & memoryBits;
        const VkMemoryPropertyFlags propFlags = memoryProperties.memoryTypes[i].propertyFlags;
        const bool hasRequiredProperties = (propFlags & properties) == properties;
        if (isRequiredMemType && hasRequiredProperties)
        {
            memoryIndex = i;
            break;
        }
        else if (i == memoryProperties.memoryTypeCount - 1)
        {
            ERR("Couldn't find a suitable memory type for an allocation");
        }
    }
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = memReq.size;
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.memoryTypeIndex = memoryIndex;
    VkDeviceMemory deviceMemory;
    if (vkAllocateMemory(device, &allocInfo, ALLOCATOR, &deviceMemory) != VK_SUCCESS)
    {
        ERR("Failed to allocate memory");
    }
    return deviceMemory;
}
vector<char> Application::readBin(const char *path)
{
    vector<char> data;
    fstream fileStream(path, ios::in | ios::binary | ios::ate);
    if (fileStream.is_open())
    {
        data.resize(fileStream.tellg());
        fileStream.seekg(0, ios::beg);
        fileStream.read(data.data(), data.size());
    }
    else
    {
        ERR(FORMAT("Failed to open file {}", path));
    }
    return data;
}
void Application::terminate()
{
    vkDestroyBuffer(device, mesh->vertexBuffer, ALLOCATOR);
    vkFreeMemory(device, mesh->vertexBufferMem, ALLOCATOR);
    vkDestroyPipeline(device, pipeline, ALLOCATOR);
    vkDestroyPipelineLayout(device, pipelineLayout, ALLOCATOR);
    vkDestroyDescriptorSetLayout(device, dsl, ALLOCATOR);
    for (uint32_t i = 0; i < framebuffers.size(); ++i)
    {
        vkDestroyFramebuffer(device, framebuffers[i].vkHandle, ALLOCATOR);
        vkDestroyImageView(device, framebuffers[i].swapchainImgView, ALLOCATOR);
        vkDestroyImageView(device, framebuffers[i].depthImgView, ALLOCATOR);
        vkDestroyImage(device, framebuffers[i].depthImg, ALLOCATOR);
        vkFreeMemory(device, framebuffers[i].depthImgMemory, ALLOCATOR);
    }
    vkDestroyRenderPass(device, renderPass, ALLOCATOR);
    vkDestroyDescriptorPool(device, descriptorPool, ALLOCATOR);
    vkDestroyCommandPool(device, cmdPool, ALLOCATOR);
    vkDestroySwapchainKHR(device, swapchain, ALLOCATOR);
    vkDestroyDevice(device, ALLOCATOR);
    vkDestroySurfaceKHR(vkInstance, surface, ALLOCATOR);
    vkDestroyInstance(vkInstance, ALLOCATOR);
    glfwDestroyWindow(window);
    glfwTerminate();
    delete mesh;
    LOG("Application exited successfully");
}