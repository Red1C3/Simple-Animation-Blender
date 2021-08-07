#pragma once
#include <vector>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <simple-animation-blender/Logger.h>
#define STR VK_STRUCTURE_TYPE
#define ALLOCATOR nullptr
class Application
{
private:
    char *meshPath;
    GLFWwindow *window;
    VkInstance vkInstance;
    VkPhysicalDevice physicalDevice;
    bool debug = true; //TODO false on release
    Application();
    void createWindow(int height, int width);
    void createVkInstance();
    void createPhysicalDevice();

public:
    static Application &instance();
    void init(char *meshPath);
    void terminate();
};