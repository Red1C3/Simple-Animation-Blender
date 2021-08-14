#include <simple-animation-blender/Application.h>
int main(int argc, char **argv)
{
    if (argc != 2)
    {
        ERR("Mesh data was not provided, please provide it as a CLI argument");
    }
    Application::instance().init(argv[1]);
    Application::instance().mainLoop();
    Application::instance().terminate();
}