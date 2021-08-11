#pragma once
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <simple-animation-blender/Logger.h>
#define STR VK_STRUCTURE_TYPE
#define ALLOCATOR nullptr
struct Framebuffer
{
    VkFramebuffer vkHandle;
    VkImageView swapchainImgView, depthImgView;
    VkImage depthImg;
    VkDeviceMemory depthImgMemory;
};
class Application
{
private:
    int fbHeight, fbWidth;
    char *meshPath;
    GLFWwindow *window;
    VkInstance vkInstance;
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    VkSurfaceKHR surface;
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VkSurfaceFormatKHR surfaceFormat;
    VkDevice device;
    VkSwapchainKHR swapchain;
    VkCommandPool cmdPool;
    VkDescriptorPool descriptorPool;
    VkRenderPass renderPass;
    std::vector<Framebuffer> framebuffers;
    uint32_t graphicsQueueFamilyIndex, presentQueueFamilyIndex,
        graphicsQueuesCount, presentQueuesCount;
    bool sameQueueForGraphicsAndPresent = false;
    bool debug = true; //TODO false on release
    Application();
    void createWindow(int height, int width);
    void createVkInstance();
    void createPhysicalDevice();
    void createSurface();
    void createQueuesFamilies();
    void createLogicalDevice();
    VkSwapchainKHR createSwapchain(VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
    void createCommandPool();
    void createDescriptorPool();
    void createRenderPass();
    void createFramebuffers();
    VkDeviceMemory allocateMemory(VkMemoryRequirements memReq, VkMemoryPropertyFlags properties);

public:
    static Application &instance();
    void init(char *meshPath);
    void terminate();
};