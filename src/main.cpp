#include <simple-animation-blender/Application.h>
#include <simple-animation-blender/GUI.h>
int main(int argc, char **argv)
{
    if (argc != 2)
    {
        ERR("Mesh data was not provided, please provide it as a CLI argument");
    }
    Application::instance().init(argv[1]);
    GUI::instance().init();
    float colorHolder[3] = {1.0f, 0.0f, 0.0f};
    {
        VkSemaphore acquireSemaphore{};
        VkSemaphoreCreateInfo semaphoreCreateInfo{};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        if (vkCreateSemaphore(Application::instance().device, &semaphoreCreateInfo, ALLOCATOR, &acquireSemaphore) != VK_SUCCESS)
        {
            ERR("Failed to create semaphore");
        }
        VkFence fence{};
        VkFenceCreateInfo fenceCreateInfo{};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        if (vkCreateFence(Application::instance().device, &fenceCreateInfo, ALLOCATOR, &fence) != VK_SUCCESS)
        {
            ERR("Failed to create fence");
        }
        VkSemaphore presentSemaphore;
        if (vkCreateSemaphore(Application::instance().device, &semaphoreCreateInfo, ALLOCATOR, &presentSemaphore) != VK_SUCCESS)
        {
            ERR("Failed to create semaphore");
        }
        while (!glfwWindowShouldClose(Application::instance().window))
        {
            glfwPollEvents();
            GUI::instance().updateColor(colorHolder);
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            ImGui::Begin("Simple Animation Blender");
            ImGui::ColorEdit3("Mesh Color", colorHolder);
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
            ImGui::Render();
            ImDrawData *imGuiDrawData = ImGui::GetDrawData();

            vkWaitForFences(Application::instance().device, 1, &fence, VK_TRUE, UINT64_MAX);

            uint32_t imageIndex;
            vkAcquireNextImageKHR(Application::instance().device, Application::instance().swapchain, 0, acquireSemaphore, VK_NULL_HANDLE, &imageIndex);
            {
                if (vkResetCommandPool(Application::instance().device, Application::instance().cmdPool, 0) != VK_SUCCESS)
                {
                    ERR("Failed to reset cmd pool");
                }
                VkCommandBufferBeginInfo beginInfo{};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

                VkRenderPassBeginInfo rpBeginInfo{};
                rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                rpBeginInfo.renderPass = Application::instance().renderPass;
                rpBeginInfo.framebuffer = Application::instance().framebuffers[imageIndex].vkHandle;
                rpBeginInfo.clearValueCount = 2;
                VkClearValue clearValues[2];
                clearValues[0].color = {0.25f, 0.25f, 0.25f, 1};
                clearValues[1].depthStencil.depth = 1.0f;
                rpBeginInfo.pClearValues = clearValues;
                rpBeginInfo.renderArea.extent.height = Application::instance().fbHeight;
                rpBeginInfo.renderArea.extent.width = Application::instance().fbWidth;
                rpBeginInfo.renderArea.offset = {0, 0};
                VkDeviceSize offsets = 0;
                if (vkBeginCommandBuffer(Application::instance().cmdBuffers[imageIndex], &beginInfo) != VK_SUCCESS)
                {
                    ERR(FORMAT("Failed to create cmd buffer {}", imageIndex));
                }
                vkCmdBeginRenderPass(Application::instance().cmdBuffers[imageIndex], &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
                vkCmdBindPipeline(Application::instance().cmdBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, Application::instance().pipeline);
                vkCmdBindVertexBuffers(Application::instance().cmdBuffers[imageIndex], 0, 1, &(Application::instance().mesh->vertexBuffer), &offsets);
                vkCmdBindIndexBuffer(Application::instance().cmdBuffers[imageIndex], Application::instance().mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT16);
                vkCmdBindDescriptorSets(Application::instance().cmdBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, Application::instance().pipelineLayout, 0, 1, &(Application::instance().mesh->descriptorSet), 0, nullptr);
                vkCmdDrawIndexed(Application::instance().cmdBuffers[imageIndex], Application::instance().mesh->indices.size(), 1, 0, 0, 0);
                ImGui_ImplVulkan_RenderDrawData(imGuiDrawData, Application::instance().cmdBuffers[imageIndex]);
                vkCmdEndRenderPass(Application::instance().cmdBuffers[imageIndex]);
                if (vkEndCommandBuffer(Application::instance().cmdBuffers[imageIndex]) != VK_SUCCESS)
                {
                    ERR(FORMAT("Failed to record cmd buffer {}", imageIndex));
                }
            }
            VkSubmitInfo submitInfo{};
            submitInfo.commandBufferCount = 1;
            VkCommandBuffer cmdBuffer = Application::instance().cmdBuffers[imageIndex];
            submitInfo.pCommandBuffers = &cmdBuffer;
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = &acquireSemaphore;
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            VkPipelineStageFlags waitStage[] = {VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};
            submitInfo.pWaitDstStageMask = waitStage;
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &presentSemaphore;
            vkResetFences(Application::instance().device, 1, &fence);
            if (vkQueueSubmit(Application::instance().graphicsQueue, 1, &submitInfo, fence) != VK_SUCCESS)
            {
                ERR("Failed to submit graphics render");
            }
            VkPresentInfoKHR presentInfo{};
            presentInfo.pImageIndices = &imageIndex;
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &presentSemaphore;
            presentInfo.swapchainCount = 1;
            VkSwapchainKHR swapchain = Application::instance().swapchain;
            presentInfo.pSwapchains = &swapchain;
            if (vkQueuePresentKHR(Application::instance().graphicsQueue, &presentInfo) != VK_SUCCESS)
            {
                ERR("Failed to present to swapchain");
            }
        }
        vkQueueWaitIdle(Application::instance().graphicsQueue);
        vkDestroyFence(Application::instance().device, fence, ALLOCATOR);
        vkDestroySemaphore(Application::instance().device, acquireSemaphore, ALLOCATOR);
        vkDestroySemaphore(Application::instance().device, presentSemaphore, ALLOCATOR);
    }
    GUI::instance().terminate();
    Application::instance().terminate();
}