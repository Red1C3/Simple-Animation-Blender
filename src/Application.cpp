#include <simple-animation-blender/Application.h>
Application::Application()
{
}
Application &Application::instance()
{
    static Application app;
    return app;
}
void Application::init()
{
    setupWindow(720, 1280);
}
void Application::setupWindow(int height, int width)
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