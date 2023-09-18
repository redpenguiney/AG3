#pragma once
#include "../../external_headers/GLEW/glew.h"
#include "../../external_headers/GLFW/glfw3.h"
#include "../../external_headers/GLM/ext.hpp"
#include "gl_error_handler.cpp"

#include <cstdio>
#include <cstdlib>
#include <unordered_map>

// User input stuff.
// index is key enums provided by GLFW
// TODO: these maps are def not thread safe, probably needs a rwlock
std::unordered_map<unsigned int, bool> PRESSED_KEYS;
std::unordered_map<unsigned int, bool> PRESS_BEGAN_KEYS;
std::unordered_map<unsigned int, bool> PRESS_ENDED_KEYS;

glm::dvec2 MOUSE_POS;
glm::dvec2 MOUSE_DELTA; // how much mouse has moved since last frame

class Window {
    public:
    static inline int width = 0;
    static inline int height = 0;
    bool mouseLocked;
    Window(int widthh, int heightt) {
        width = widthh;
        height = heightt;
        mouseLocked = false;
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
        glfwSetKeyCallback(glfwWindow, KeyCallback);
        glfwSetFramebufferSizeCallback(glfwWindow, ResizeCallback);
        glfwSwapInterval(1); // do vsync
        
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
        PRESS_BEGAN_KEYS.clear(); // TODO: might have problems with this, idk how key callback really works
        PRESS_ENDED_KEYS.clear();

        glfwSwapBuffers(glfwWindow);
		glfwPollEvents();
        glm::dvec2 pos;
        glfwGetCursorPos(glfwWindow, &pos.x, &pos.y);
        MOUSE_DELTA = pos - MOUSE_POS;
        MOUSE_POS = pos;
    }

    // returns true if the user is trying to close the application, or if Window::Close() was explicitly called (like by a quit game button)
    bool ShouldClose() {
        return glfwWindowShouldClose(glfwWindow);
    }

    void Close() {
        glfwSetWindowShouldClose(glfwWindow, true);
    } 

    // TODO: when disabling mouse lock MOUSE_DELTA has a weird value
    void SetMouseLocked(bool locked) {
        mouseLocked = locked;
        glfwSetInputMode(glfwWindow, GLFW_CURSOR, (locked) ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

    }

    private:
    GLFWwindow* glfwWindow;

    // GLFW calls these functions automatically 
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS) {
            PRESS_BEGAN_KEYS[key] = true;
            PRESSED_KEYS[key] = true;
        }
        else if (action == GLFW_RELEASE) {
            PRESS_ENDED_KEYS[key] = true;
            PRESSED_KEYS[key] = false;
        }
    } 

    static void ResizeCallback(GLFWwindow* window, int newWindowWidth, int newWindowHeight) { // called on window resize
        // Tell OpenGL to draw to the whole screen
        glViewport(0, 0, newWindowWidth, newWindowHeight);
        width = newWindowWidth;
        height = newWindowHeight;
    }
};

