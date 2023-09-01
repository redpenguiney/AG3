#include "../../external_headers/GLEW/glew.h"
#include "../../external_headers/GLFW/glfw3.h"

#include <cstdio>
#include <cstdlib>

class Window {
    public:
    Window(int width, int height) {
        auto initSuccess = glfwInit();
        if (!initSuccess) {
            std::printf("\nFailure to initialize GLFW. Aborting.");
            abort();
        }

        glfwWindow = glfwCreateWindow(width, height, "AG3", nullptr, nullptr);
        if (!glfwWindow) {
            glfwTerminate();
            std::printf("\nFailure to create GLFW window. Aborting.");
            abort();
        }

        glfwMakeContextCurrent(glfwWindow);
        
        GLenum glewSuccess = glewInit();
        if (glewSuccess != GLEW_OK) {
            glfwTerminate();
            std::printf("\nFailure to initalize GLEW (error %s). Aborting.", glewGetErrorString(glewSuccess));
            abort();
        }

        printf("\nWindow creation successful.");
    };

    ~Window() {
        glfwTerminate();
    }

    void Update() {
        glfwSwapBuffers(glfwWindow);
		glfwPollEvents();
    }

    // returns true if the user is trying to close the application, or if glfwSetWindowShouldClose was explicitly called (like by a quit game button)
    bool ShouldClose() {
        return glfwWindowShouldClose(glfwWindow);
    }

    private:
    GLFWwindow* glfwWindow;
};