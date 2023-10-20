#pragma once
#include "../../external_headers/GLEW/glew.h"
#include "../../external_headers/GLFW/glfw3.h"
#include "../../external_headers/GLM/ext.hpp"
#include <unordered_map>

// User input stuff.
// index is key enums provided by GLFW
// TODO: these maps are def not thread safe, probably needs a rwlock
inline std::unordered_map<unsigned int, bool> PRESSED_KEYS;
inline std::unordered_map<unsigned int, bool> PRESS_BEGAN_KEYS;
inline std::unordered_map<unsigned int, bool> PRESS_ENDED_KEYS;

inline bool LMB_DOWN;
inline bool RMB_DOWN;

inline glm::dvec2 MOUSE_POS;
inline glm::dvec2 MOUSE_DELTA; // how much mouse has moved since last frame

class Window {
    public:
    static inline int width = 0;
    static inline int height = 0;
    bool mouseLocked;

    Window() = delete; 

    Window(int widthh, int heightt);
    ~Window();

    void Update();
    bool ShouldClose();
    void Close();
    void SetMouseLocked(bool locked);

    private:
    GLFWwindow* glfwWindow;
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void ResizeCallback(GLFWwindow* window, int newWindowWidth, int newWindowHeight);

};