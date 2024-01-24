#pragma once

// Singleton that handles loading and running lua scripts.
class LuaHandler {
    public:
    static LuaHandler& Get();

    LuaHandler(LuaHandler const&) = delete; // no copying
    LuaHandler& operator=(LuaHandler const&) = delete; // no assigning

    private:
    LuaHandler();
    ~LuaHandler();
};