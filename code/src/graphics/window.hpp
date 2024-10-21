#pragma once
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "GLM/vec2.hpp"
#include <unordered_set>
#include "events/event.hpp"

// Object representing any kind of input.
class InputObject {
    public:

    enum InputType {
        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,

        One,
        Two,
        Three,
        Four,
        Five,
        Six,
        Seven,
        Eight,
        Nine,
        Zero,

        Space,
        Tab,
        Escape,
        Grave,
        LBracket,
        RBracket,

        Ctrl,
        Shift,
        Alt,

        LMB,// mouse button 1
        RMB,// mouse button 2
        MMB,// mouse button 3
        MB4,
        MB5,
        MB6,
        MB7,
        MB8,

        ScrollUp,
        ScrollDown,

        Unknown
    };

    InputType input;

    bool capitalized; // if it's a letter, whether it's capitalized, accounting for both shift and capslock
    bool altDown;
    bool ctrlDown;
    bool shiftDown;

    bool operator== (const InputObject&) const = default;
};

// hash inputobject so it can go in unordered map
template <>
struct std::hash<InputObject> {
    std::size_t operator()(const InputObject& io) const noexcept {
        size_t h1 = io.capitalized + io.altDown * 8 + io.ctrlDown * 256 + io.shiftDown * 2048;
        size_t h2 = io.input;
        return h1 ^ (h2 << 1);
    }
};

class Window {
    public:
    unsigned int width = 0;
    unsigned int height = 0;
    
    std::shared_ptr<Event<InputObject>> inputDown;
    std::shared_ptr<Event<InputObject>> inputUp;
    
    // fired after Update() finishes
    std::shared_ptr<Event<>> postInputProccessing;

    // fired during Update() before postInputProccesing if the window was resized that frame.
    // first uvec2 is old window size, secon is new window size
    std::shared_ptr<Event<glm::uvec2, glm::uvec2>> onWindowResize;

    Window() = delete; 

    Window(int widthh, int heightt);
    ~Window();

    float Aspect() const {return float(width)/float(height);}

    // Processes user input and fires PostInputProcessing
    void Update();

    // If VSync is enabled, will yield until the frame can be displayed.
    // If VSync is disabled, I have no idea what happens (TODO)
    void FlipBuffers();

    // Returns true if the user is trying to close the window or Close() was called.
    bool ShouldClose();

    // Doesn't immediately close it, but will make all subsequent calls to ShouldClose() return true (meaning the program will exit at end of this frame)
    void Close();

    
    void SetMouseLocked(bool locked);
    bool IsMouseLocked() const;

    // User input stuff.
    // index is key enums provided by GLFW
    // TODO: these maps are def not thread safe, probably needs a rwlock
    std::unordered_set<InputObject::InputType> PRESSED_KEYS;
    std::unordered_set<InputObject> PRESS_BEGAN_KEYS;
    std::unordered_set<InputObject> PRESS_ENDED_KEYS;

    /*bool IsPressed(int key);
    bool IsPressBegan(int key);
    bool IsPressEnded(int key);*/

    /*bool LMB_DOWN = false;
    bool RMB_DOWN = false;
    bool LMB_BEGAN = false;
    bool RMB_BEGAN = false;
    bool LMB_ENDED = false;
    bool RMB_ENDED = false;*/

    //bool SHIFT_DOWN; // TODO
    //bool CTRL_DOWN; // TODO

    // TODO: RAW MOUSE option
    glm::dvec2 MOUSE_POS = {0, 0};
    glm::dvec2 MOUSE_DELTA = {0, 0}; // how much mouse has moved since last frame

    private:
    bool mouseLocked = false;

    GLFWwindow* glfwWindow;
    // callbacks are called when glfwPollEvents() is called (in Update())
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void ResizeCallback(GLFWwindow* window, int newWindowWidth, int newWindowHeight);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
};