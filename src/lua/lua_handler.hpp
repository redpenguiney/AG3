#pragma once
#include <string>

// Singleton that handles loading and running lua scripts.
class LuaHandler {
    public:
    static LuaHandler& Get();

    // When modules (shared libraries) get their copy of this code, they need to use a special version of LuaHandler::Get().
    // This is so that both the module and the main executable have access to the same singleton. 
    // The executable will provide each shared_library with a pointer to the lua handler.
    #ifdef IS_MODULE
    static void SetModuleLuaHandler(LuaHandler* engine);
    #endif

    LuaHandler(LuaHandler const&) = delete; // no copying
    LuaHandler& operator=(LuaHandler const&) = delete; // no assigning

    void RunString(const std::string source);
    void RunFile(const std::string scriptPath);

    void PreRenderCallbacks();
    void PrePhysicsCallbacks();
    void PostRenderCallbacks();
    void PostPhysicsCallbacks();

    void OnFrameBegin();

    private:

    LuaHandler();
    ~LuaHandler();
};