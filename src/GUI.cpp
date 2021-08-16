#include <simple-animation-blender/GUI.h>
using namespace glm;
GUI::GUI() {}
GUI &GUI::instance()
{
    static GUI gui;
    return gui;
}
void GUI::init()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForVulkan(Application::instance().window, true);
    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = Application::instance().vkInstance;
    initInfo.PhysicalDevice = Application::instance().physicalDevice;
    initInfo.Device = Application::instance().device;
    initInfo.QueueFamily = Application::instance().graphicsQueueFamilyIndex;
    initInfo.Queue = Application::instance().graphicsQueue;
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = Application::instance().descriptorPool;
    initInfo.Allocator = ALLOCATOR;
    initInfo.ImageCount = Application::instance().framebuffers.size();
    initInfo.MinImageCount = Application::instance().surfaceCapabilities.minImageCount;
    ImGui_ImplVulkan_Init(&initInfo, Application::instance().renderPass);
    {
        if (vkResetCommandPool(Application::instance().device, Application::instance().cmdPool, 0) != VK_SUCCESS)
        {
            ERR("Failed to reset cmd pool");
        }
        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (vkBeginCommandBuffer(Application::instance().cmdBuffers[0], &begin_info) != VK_SUCCESS)
        {
            ERR("Failed to begin fonts cmd buffer");
        }
        ImGui_ImplVulkan_CreateFontsTexture(Application::instance().cmdBuffers[0]);

        VkSubmitInfo end_info = {};
        end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        end_info.commandBufferCount = 1;
        end_info.pCommandBuffers = &Application::instance().cmdBuffers[0];
        if (vkEndCommandBuffer(Application::instance().cmdBuffers[0]) != VK_SUCCESS)
        {
            ERR("Failed to end fonts cmd buffer");
        }
        if (vkQueueSubmit(Application::instance().graphicsQueue, 1, &end_info, VK_NULL_HANDLE) != VK_SUCCESS)
        {
            ERR("Failed to submit fonts cmd buffer");
        }

        if (vkDeviceWaitIdle(Application::instance().device) != VK_SUCCESS)
        {
            ERR("Failed waiting on device while submiting fonts cmd buffer");
        }
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
}
void GUI::updateColor(float newColor[3])
{
    vec3 currentColor = Application::instance().mesh->ubo.color;
    if (currentColor.r == newColor[0] && currentColor.g == newColor[1] && currentColor.b == newColor[2])
        return;
    currentColor = vec3(newColor[0], newColor[1], newColor[2]);
    Application::instance().mesh->ubo.color = currentColor;
    Application::instance().updateUBO(Application::instance().mesh->ubo);
}
void GUI::terminate()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}