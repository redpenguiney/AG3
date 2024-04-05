#include "lua_handler.hpp"
// #define SOL_USE_LUA_HPP
#include <sol/sol.hpp>
#include <cassert>
#include <optional>

#include "../gameobjects/component_registry.hpp"
#include "../debug/log.hpp"

// normally this would be a private member of the lua handler class, but then the header file would have to include all of sol2 which would probably make compile times worse
// we use sol2 for lua support.
std::optional<sol::state> LUA_STATE = std::nullopt;

LuaHandler& LuaHandler::Get() {
    static LuaHandler lh;
    return lh;
}

LuaHandler::LuaHandler() {
    assert(!LUA_STATE.has_value()); // the constructor is supposed to make a lua state so there better not already be one
    
    // intitialize lua
    LUA_STATE = sol::state();

    // setup lua standard library
    LUA_STATE->open_libraries(sol::lib::base, sol::lib::os, sol::lib::math, sol::lib::table, sol::lib::coroutine, sol::lib::string);

    // expose our stuff to lua
    LUA_STATE->set("GE", &GraphicsEngine::Get());
    LUA_STATE->set("NewGameObject", ComponentRegistry::NewGameObject);
    // LUA_STATE->set("ComponentBitIndex", &GraphicsEngine::Get());
}

void LuaHandler::RunString(const std::string source) {
    
    LUA_STATE->safe_script(source);
}

void LuaHandler::RunFile(const std::string scriptPath) {
    // "safe" means it won't just throw if the script doesn't successfully run to completion
    auto result = LUA_STATE->safe_script_file(scriptPath, [](lua_State*, sol::protected_function_result _result) {
        DebugLogError("While attempting to run lua file:\n", ((sol::error)_result).what());

        return _result;
    });
    
}

LuaHandler::~LuaHandler() {
    LUA_STATE = std::nullopt; // delete all the lua stuff and let sol2 cleanup
}