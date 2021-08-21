/*MIT License

Copyright (c) 2021 Mohammad Issawi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/
#pragma once
#include <vector>
#include <fstream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <simple-animation-blender/Logger.h>
#include <simple-animation-blender/Mesh.h>
#define STR VK_STRUCTURE_TYPE
#define ALLOCATOR nullptr

class Application
{
private:
    struct Framebuffer
    {
        VkFramebuffer vkHandle;
        VkImageView swapchainImgView, depthImgView;
        VkImage depthImg;
        VkDeviceMemory depthImgMemory;
    };
    char *meshPath;
    float zoom = 10.0f;
    glm::mat4 persp, clip;
    VkInstance vkInstance;
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    VkSurfaceKHR surface;
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VkSurfaceFormatKHR surfaceFormat;
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout dsl;

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
    void createDescriptorSetLayout();
    void createPipelineLayout();
    void createPipeline();
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffer();
    void createCommandBuffers();
    void recordCommandBuffers();
    void allocateDescriptorSet();
    void updateUBO(Mesh::UBO &ubo);
    VkDeviceMemory allocateMemory(VkMemoryRequirements memReq, VkMemoryPropertyFlags properties);
    std::vector<char> readBin(const char *path);

public:
    int fbHeight, fbWidth;
    GLFWwindow *window;
    Mesh *mesh;
    VkDevice device;
    VkCommandPool cmdPool;
    VkRenderPass renderPass;
    VkSwapchainKHR swapchain;
    std::vector<VkCommandBuffer> cmdBuffers;
    VkQueue graphicsQueue;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    std::vector<Framebuffer> framebuffers;
    static Application &instance();
    void init(char *meshPath);
    void mainLoop();
    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
    void terminate();
    friend class GUI;
    friend class Animator;
};