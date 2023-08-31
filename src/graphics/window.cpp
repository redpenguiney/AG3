#include "../../external_headers/GLEW/include/glew.h"
#include "../../external_headers/GLFW/include/glfw3.h"

#include <cstdio>
#include <cstdlib>

class Window {
    public:
    Window(int width, int height) {
        auto initSuccess = glfwInit();
        if (!initSuccess) {
            std::printf("\nFailure to initialize GLFW.");
            abort();
        }

        glfwCreateWindow(width, height, "AG3", nullptr, glfwWindow);
        glfwMakeContextCurrent(glfwWindow);
        
    };

    private:
    GLFWwindow* glfwWindow;
};