#pragma once
#include "../../external_headers/GLEW/glew.h"
#include "../../external_headers/GLFW/glfw3.h"
#include "../../external_headers/GLM/ext.hpp"
#include <unordered_map>



class Window {
    public:
    unsigned int width = 0;
    unsigned int height = 0;
    bool mouseLocked;

    Window() = delete; 

    Window(int widthh, int heightt);
    ~Window();

    float Aspect() const {return float(width)/float(height);}

    // Processes user input
    void Update();

    // If VSync is enabled, will yield until the frame can be displayed.
    // If VSync is disabled, I have no idea what happens (TODO)
    void FlipBuffers();

    // Returns true if the user is trying to close the window or Close() was called.
    bool ShouldClose();

    // Doesn't immediately close it, but will make all subsequent calls to ShouldClose() return true (meaning the program will exit at end of this frame)
    void Close();

    
    void SetMouseLocked(bool locked);

    // User input stuff.
    // index is key enums provided by GLFW
    // TODO: these maps are def not thread safe, probably needs a rwlock
    std::unordered_map<unsigned int, bool> PRESSED_KEYS;
    std::unordered_map<unsigned int, bool> PRESS_BEGAN_KEYS;
    std::unordered_map<unsigned int, bool> PRESS_ENDED_KEYS;

    bool LMB_DOWN;
    bool RMB_DOWN;
    bool LMB_BEGAN;
    bool RMB_BEGAN;
    bool LMB_ENDED;
    bool RMB_ENDED;

    // TODO: RAW MOUSE option
    glm::dvec2 MOUSE_POS;
    glm::dvec2 MOUSE_DELTA; // how much mouse has moved since last frame

    private:
    GLFWwindow* glfwWindow;
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void ResizeCallback(GLFWwindow* window, int newWindowWidth, int newWindowHeight);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
};