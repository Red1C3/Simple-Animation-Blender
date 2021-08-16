#pragma once
#include <vector>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <simple-animation-blender/Application.h>
class GUI
{
private:
    GUI();

public:
    void init();
    void updateColor(float newColor[3]);
    void terminate();
    static GUI &instance();
};