#pragma once
#include <string>

// Singleton that handles loading and running lua scripts.
class LuaHandler {
    public:
    static LuaHandler& Get();

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