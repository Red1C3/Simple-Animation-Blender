#pragma once
#include <GLFW/glfw3.h>
#include<simple-animation-blender/Logger.h>
class Application
{
private:
    GLFWwindow *window;
    Application();
    void setupWindow(int height, int width);

public:
    static Application &instance();
    void init();
};