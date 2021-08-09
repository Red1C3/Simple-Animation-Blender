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
    createSurface();
    createQueuesFamilies();
    createLogicalDevice();
    swapchain = createSwapchain();
    //TODO create command pool
    //TODO create descriptor pool
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
void Application::terminate()
{
    vkDestroySwapchainKHR(device, swapchain, ALLOCATOR);
    vkDestroyDevice(device, ALLOCATOR);
    vkDestroySurfaceKHR(vkInstance, surface, ALLOCATOR);
    vkDestroyInstance(vkInstance, ALLOCATOR);
    glfwDestroyWindow(window);
    glfwTerminate();
    LOG("Application exited successfully");
}