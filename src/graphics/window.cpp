#include "window.hpp"
#include "gl_error_handler.cpp"

#include <cstdio>
#include <cstdlib>

Window::Window(int widthh, int heightt) {
    width = widthh;
    height = heightt;
    mouseLocked = false;
    auto initSuccess = glfwInit();
    if (!initSuccess) {
        std::printf("Failure to initialize GLFW. Aborting.\n");
        abort();
    }

    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE); // Tell GLFW we are going to be running opengl in debug mode, which lets us use GL_DEBUG_OUTPUT to get error messages easily
    glfwWindow = glfwCreateWindow(width, height, "AG3", nullptr, nullptr);
    if (!glfwWindow) {
        glfwTerminate();
        std::printf("Failure to create GLFW window. Aborting.\n");
        abort();
    }

    glfwMakeContextCurrent(glfwWindow);
    glfwSetKeyCallback(glfwWindow, KeyCallback);
    glfwSetMouseButtonCallback(glfwWindow, MouseButtonCallback);
    glfwSetFramebufferSizeCallback(glfwWindow, ResizeCallback);
    glfwSwapInterval(1); // do vsync
    
    GLenum glewSuccess = glewInit();
    if (glewSuccess != GLEW_OK) {
        glfwTerminate();
        std::printf("Failure to initalize GLEW (error %s). Aborting.\n", glewGetErrorString(glewSuccess));
        abort();
    }

    glfwSetWindowPos(glfwWindow, 540, 180);

    printf("Window creation successful.\n");

    // See gl_error_handler, just prints opengl errors to console automatically
    // todo: disable on release builds for performance
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);
};

Window::~Window() {
    glfwTerminate();
}

void Window::Update() {
    PRESS_BEGAN_KEYS.clear(); // TODO: might have problems with this, idk how key callback really works
    PRESS_ENDED_KEYS.clear();
    LMB_BEGAN = false;
    LMB_ENDED = false;
    RMB_BEGAN = false;
    RMB_ENDED = false;

    glfwPollEvents();
    glm::dvec2 pos;
    glfwGetCursorPos(glfwWindow, &pos.x, &pos.y);
    MOUSE_DELTA = pos - MOUSE_POS;
    MOUSE_POS = pos;
}

void Window::FlipBuffers() {
    glfwSwapBuffers(glfwWindow);
}

// returns true if the user is trying to close the application, or if Window::Close() was explicitly called (like by a quit game button)
bool Window::ShouldClose() {
    return glfwWindowShouldClose(glfwWindow);
}

void Window::Close() {
    glfwSetWindowShouldClose(glfwWindow, true);
}

// TODO: when disabling mouse lock MOUSE_DELTA has a weird value
void Window::SetMouseLocked(bool locked) {
    mouseLocked = locked;
    glfwSetInputMode(glfwWindow, GLFW_CURSOR, (locked) ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

}

// GLFW calls these functions automatically 
void Window::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        PRESS_BEGAN_KEYS[key] = true;
        PRESSED_KEYS[key] = true;
    }
    else if (action == GLFW_RELEASE) {
        PRESS_ENDED_KEYS[key] = true;
        PRESSED_KEYS[key] = false;
    }
} 

void Window::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_1) {
        if (action == GLFW_RELEASE) {
            LMB_DOWN = false;
            LMB_ENDED = true;
        }
        else if (action == GLFW_PRESS) {
            LMB_DOWN = true;
            LMB_BEGAN = true;

        }
    }
    else if (button == GLFW_MOUSE_BUTTON_2) {
        if (action == GLFW_RELEASE) {
            RMB_DOWN = false;
            RMB_ENDED = true;
        }
        else if (action == GLFW_PRESS) {
            RMB_DOWN = true;
            RMB_BEGAN = true;
        }
    }
}

void Window::ResizeCallback(GLFWwindow* window, int newWindowWidth, int newWindowHeight) { // called on window resize
    // Tell OpenGL to draw to the whole screen
    glViewport(0, 0, newWindowWidth, newWindowHeight);
    width = newWindowWidth;
    height = newWindowHeight;
}


