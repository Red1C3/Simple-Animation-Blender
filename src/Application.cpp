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
    //TODO create logical device
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
    VkPhysicalDevice integratedFallback;
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
void Application::terminate()
{
    vkDestroyInstance(vkInstance, ALLOCATOR);
    glfwDestroyWindow(window);
    glfwTerminate();
    LOG("Application exited successfully");
}