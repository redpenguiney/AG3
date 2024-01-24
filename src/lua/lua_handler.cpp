#include "lua_handler.hpp"
// #define SOL_USE_LUA_HPP
// #include "../../external_headers/SOL2/include/sol/sol.hpp"
#include <cassert>
#include <optional>

// normally this would be a private member of the lua handler class, but then the header file would have to include all of sol2 which would probably make compile times worse
// we use sol2 for lua support.
// std::optional<sol::state> LUA_STATE = std::nullopt;

LuaHandler& LuaHandler::Get() {
    static LuaHandler lh;
    return lh;
}

LuaHandler::LuaHandler() {
    // assert(!LUA_STATE.has_value()); // the constructor is supposed to make a lua state so there better not already be one
    
    // intitialize lua
    // LUA_STATE = sol::state();
}

LuaHandler::~LuaHandler() {
    // LUA_STATE = std::nullopt; // delete all the lua stuff and let sol2 cleanup
}