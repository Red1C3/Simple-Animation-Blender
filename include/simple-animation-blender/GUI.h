#pragma once
#include <vector>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <simple-animation-blender/Application.h>
#include <simple-animation-blender/Animator.h>
class GUI
{
private:
    GUI();
    int currentFirstPlayingAnimation, currentSecondPlayingAnimation;
    float currentBlendingFactor;

public:
    void init();
    void updateColor(float newColor[3]);
    void updateAnimatorData(int firstAnim, int secondAnim, float blendingFactor);
    void terminate();
    static GUI &instance();
};