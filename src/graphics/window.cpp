#include "../../external_headers/GLEW/glew.h"
#include "../../external_headers/GLFW/glfw3.h"
#include "gl_error_handler.cpp"

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

        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE); // Tell GLFW we are going to be running opengl in debug mode, which lets us use GL_DEBUG_OUTPUT to get error messages easily
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

        // See gl_error_handler, just prints opengl errors to console automatically
        // todo: disable on release builds for performance
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(MessageCallback, 0);
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