#include "window.hpp"
#include "gl_error_handler.cpp"
#include "../conglomerates/gui.hpp"

#include <cstdio>
#include <cstdlib>

Window::Window(int widthh, int heightt):
    inputDown(Event<InputObject>::New()),
    inputUp(Event<InputObject>::New()),
    onScroll(Event<double, double>::New()),
    postInputProccessing(Event<>::New()),
    onWindowResize(Event<glm::uvec2, glm::uvec2>::New())
{
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
    glfwSetScrollCallback(glfwWindow, ScrollCallback);
    glfwSetMouseButtonCallback(glfwWindow, MouseButtonCallback);
    glfwSetFramebufferSizeCallback(glfwWindow, ResizeCallback);
    glfwSwapInterval(1); // do vsync
    
    GLenum glewSuccess = glewInit();
    if (glewSuccess != GLEW_OK) {
        glfwTerminate();
        std::printf("Failure to initalize GLEW (error %s). Aborting.\n", glewGetErrorString(glewSuccess));
        abort();
    }

    // TODO: remove
    glfwSetWindowPos(glfwWindow, 540, 180);

    // initialize mouse position
    glfwGetCursorPos(glfwWindow, &MOUSE_POS.x, &MOUSE_POS.y);
    // std::printf("Init mouse at %f %f\n", MOUSE_POS.x, MOUSE_POS.y);

    // tell glfw we care about capslock and numpad
    glfwSetInputMode(glfwWindow, GLFW_LOCK_KEY_MODS, GLFW_TRUE);

    DebugLogInfo("Window creation successful.");

    // See gl_error_handler, just prints opengl errors to console automatically
    // todo: disable on release builds for performance
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // make sure it actually prints out the errors on the thread that created the error so you actually get a useful stack trace
    glDebugMessageCallback(MessageCallback, 0);
};

Window::~Window() {
    glfwTerminate();
}

bool Window::IsMouseLocked() const {
    return mouseLocked;
}

//bool Window::IsPressed(int key) {
//    return PRESSED_KEYS.contains(key);
//}
//bool Window::IsPressBegan(int key) {
//    return PRESS_BEGAN_KEYS.contains(key);
//}
//bool Window::IsPressEnded(int key) {
//    return PRESS_ENDED_KEYS.contains(key);
//}

void Window::Update() {
    PRESS_BEGAN_KEYS.clear(); 
    PRESS_ENDED_KEYS.clear();

    /*LMB_BEGAN = false;
    LMB_ENDED = false;
    RMB_BEGAN = false;
    RMB_ENDED = false;*/

    // set cursor pos
    glm::dvec2 pos;
    glfwGetCursorPos(glfwWindow, &pos.x, &pos.y);
    // std::printf("Old mouse pos was %f %f\n", MOUSE_POS.x, MOUSE_POS.y);
    // std::printf("Now it at %f %f\n", pos.x, pos.y);
    MOUSE_DELTA = pos - MOUSE_POS;
    MOUSE_POS = pos;

    // fire callbacks/input events
    glfwPollEvents();
    
    

    postInputProccessing->Fire();
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

InputObject::InputType glfwKeyToInputType(int key) {
    switch (key) {
    case GLFW_KEY_A:
        return InputObject::A;
    case GLFW_KEY_B:
        return InputObject::B;
    case GLFW_KEY_C:
        return InputObject::C;
    case GLFW_KEY_D:
        return InputObject::D;
    case GLFW_KEY_E:
        return InputObject::E;
    case GLFW_KEY_F:
        return InputObject::F;
    case GLFW_KEY_G:
        return InputObject::G;
    case GLFW_KEY_H:
        return InputObject::H;
    case GLFW_KEY_I:
        return InputObject::I;
    case GLFW_KEY_J:
        return InputObject::J;
    case GLFW_KEY_K:
        return InputObject::K;
    case GLFW_KEY_L:
        return InputObject::L;
    case GLFW_KEY_M:
        return InputObject::M;
    case GLFW_KEY_N:
        return InputObject::N;
    case GLFW_KEY_O:
        return InputObject::O;
    case GLFW_KEY_P:
        return InputObject::P;
    case GLFW_KEY_Q:
        return InputObject::Q;
    case GLFW_KEY_R:
        return InputObject::R;
    case GLFW_KEY_S:
        return InputObject::S;
    case GLFW_KEY_T:
        return InputObject::T;
    case GLFW_KEY_U:
        return InputObject::U;
    case GLFW_KEY_V:
        return InputObject::V;
    case GLFW_KEY_W:
        return InputObject::W;
    case GLFW_KEY_X:
        return InputObject::X;
    case GLFW_KEY_Y:
        return InputObject::Y;
    case GLFW_KEY_Z:
        return InputObject::Z;
    case GLFW_KEY_0:
        return InputObject::Zero;
    case GLFW_KEY_1:
        return InputObject::One;
    case GLFW_KEY_2:
        return InputObject::Two;
    case GLFW_KEY_3:
        return InputObject::Three;
    case GLFW_KEY_4:
        return InputObject::Four;
    case GLFW_KEY_5:
        return InputObject::Five;
    case GLFW_KEY_6:
        return InputObject::Six;
    case GLFW_KEY_7:
        return InputObject::Seven;
    case GLFW_KEY_8:
        return InputObject::Eight;
    case GLFW_KEY_9:
        return InputObject::Nine;
    case GLFW_KEY_GRAVE_ACCENT:
        return InputObject::Grave;
    case GLFW_KEY_SPACE:
        return InputObject::Space;
    case GLFW_KEY_ESCAPE:
        return InputObject::Escape;
    case GLFW_KEY_TAB:
        return InputObject::Tab;
    case GLFW_KEY_LEFT_ALT: // TODO: different alts bind to different enums???
        return InputObject::Alt;
    case GLFW_KEY_RIGHT_ALT:
        return InputObject::Alt;
    case GLFW_KEY_LEFT_BRACKET:
        return InputObject::LBracket;
    case GLFW_KEY_RIGHT_BRACKET:
        return InputObject::RBracket;
    case GLFW_KEY_LEFT_SUPER: // windows key, here to avoid an annoying unrecognized key msg 
        return InputObject::Unknown;
    case GLFW_KEY_UNKNOWN:
        DebugLogError("Unrecognized key. Even GLFW doesn't know it.");
        return InputObject::Unknown;
    default:
        DebugLogError("Unrecognized key ", key, " (GLFW recognizes it, but we don't.).");
        return InputObject::Unknown;
    }
}

// GLFW calls these functions automatically when glfwPollEvents() is called.
void Window::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {

    InputObject input{
        .input = glfwKeyToInputType(key),
        .capitalized = (mods & GLFW_MOD_CAPS_LOCK) != (mods & GLFW_MOD_SHIFT),
        .altDown = bool(mods & GLFW_MOD_ALT),
        .ctrlDown = bool(mods & GLFW_MOD_CONTROL),
        .shiftDown = bool(mods & GLFW_MOD_SHIFT)
    };

    //DebugLogInfo("INPUT = ", input.input);

    if (action == GLFW_PRESS) {
        GraphicsEngine::Get().window.PRESS_BEGAN_KEYS.insert(input);
        GraphicsEngine::Get().window.PRESSED_KEYS.insert(input.input);
        GraphicsEngine::Get().window.inputDown->Fire(input);
    }
    else if (action == GLFW_RELEASE) {
        GraphicsEngine::Get().window.PRESS_ENDED_KEYS.insert(input);
        GraphicsEngine::Get().window.PRESSED_KEYS.erase(input.input);
        GraphicsEngine::Get().window.inputUp->Fire(input);
    }
} 

void Window::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {

    InputObject input{
        .capitalized = (mods & GLFW_MOD_CAPS_LOCK) != (mods & GLFW_MOD_SHIFT),
        .altDown = bool(mods & GLFW_MOD_ALT),
        .ctrlDown = bool(mods & GLFW_MOD_CONTROL),
        .shiftDown = bool(mods & GLFW_MOD_SHIFT)
    };

    if (button > GLFW_MOUSE_BUTTON_LAST) {
        DebugLogError("Goofy mouse button value of ", button);
    }
    else {
        input.input = InputObject::InputType(InputObject::LMB + button);

        if (action == GLFW_RELEASE) {
            GraphicsEngine::Get().window.PRESS_ENDED_KEYS.insert(input);
            GraphicsEngine::Get().window.PRESSED_KEYS.erase(input.input);
            GraphicsEngine::Get().window.inputDown->Fire(input);
        }
        else if (action == GLFW_PRESS) {
            GraphicsEngine::Get().window.PRESS_BEGAN_KEYS.insert(input);
            GraphicsEngine::Get().window.PRESSED_KEYS.insert(input.input);
            GraphicsEngine::Get().window.inputUp->Fire(input);
        }
    }
    

    
}

void Window::ScrollCallback(GLFWwindow* window, double deltaScrollX, double deltaScrollY)
{
    GraphicsEngine::Get().window.onScroll->Fire(deltaScrollX, deltaScrollY);
}

void Window::ResizeCallback(GLFWwindow* window, int newWindowWidth, int newWindowHeight) { // called on window resize
    // Tell OpenGL to draw to the whole screen (TODO: Window class should not be concerned with the graphics library)
    glViewport(0, 0, newWindowWidth, newWindowHeight);

    if (newWindowWidth != 0 && newWindowHeight != 0) {
        glm::uvec2 oldWidth(GraphicsEngine::Get().window.width, GraphicsEngine::Get().window.height);
        GraphicsEngine::Get().window.width = newWindowWidth;
        GraphicsEngine::Get().window.height = newWindowHeight;

        GraphicsEngine::Get().window.onWindowResize->Fire(oldWidth, glm::uvec2(newWindowWidth, newWindowHeight));
    }
    
}