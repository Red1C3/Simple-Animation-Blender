#include <simple-animation-blender/Application.h>
using namespace std;
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
    createFramebuffer();
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
void Application::createFramebuffer()
{
    int height, width;
    glfwGetFramebufferSize(window, &width, &height);
    VkImageCreateInfo depthImgCreateInfo{};
    depthImgCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depthImgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    depthImgCreateInfo.format = VK_FORMAT_D24_UNORM_S8_UINT;
    depthImgCreateInfo.extent.depth = 1;
    depthImgCreateInfo.extent.height = height;
    depthImgCreateInfo.extent.width = width;
    depthImgCreateInfo.mipLevels = 1;
    depthImgCreateInfo.arrayLayers = 1;
    depthImgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    depthImgCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    depthImgCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depthImgCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    depthImgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(device, &depthImgCreateInfo, ALLOCATOR, &(framebuffer.depthImg)) != VK_SUCCESS)
    {
        ERR("Failed to create depth image");
    }
    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(device, framebuffer.depthImg, &memReq);
    framebuffer.depthImgMemory = allocateMemory(memReq, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkBindImageMemory(device, framebuffer.depthImg, framebuffer.depthImgMemory, 0) != VK_SUCCESS)
    {
        ERR("Failed to bind depth img with allocated memory");
    }
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
void Application::terminate()
{
    vkDestroyImage(device, framebuffer.depthImg, ALLOCATOR);
    vkFreeMemory(device, framebuffer.depthImgMemory, ALLOCATOR);
    vkDestroyRenderPass(device, renderPass, ALLOCATOR);
    vkDestroyDescriptorPool(device, descriptorPool, ALLOCATOR);
    vkDestroyCommandPool(device, cmdPool, ALLOCATOR);
    vkDestroySwapchainKHR(device, swapchain, ALLOCATOR);
    vkDestroyDevice(device, ALLOCATOR);
    vkDestroySurfaceKHR(vkInstance, surface, ALLOCATOR);
    vkDestroyInstance(vkInstance, ALLOCATOR);
    glfwDestroyWindow(window);
    glfwTerminate();
    LOG("Application exited successfully");
}