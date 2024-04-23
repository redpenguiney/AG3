// #define DEBUG_LUA
// #ifdef DEBUG_LUA
// #define SOL_ALL_SAFETIES_ON 1
// #define 
// #endif

#include "debug/log.hpp"
#include "sol/forward.hpp"
#ifndef SOL_ALL_SAFETIES_ON 
#error bruh
#endif

#include "lua_handler.hpp"
// #define SOL_USE_LUA_HPP
#include <sol/sol.hpp>
#include <cassert>
#include <optional>

#include "../gameobjects/component_registry.hpp"
#include "../debug/log.hpp"
#include "lua_constructor_wrappers.hpp"

// normally this stuff would be private members of the lua handler class, but then the header file would have to include all of sol2 which would probably make compile times worse
// we use sol2 for lua support.
std::optional<sol::state> LUA_STATE = std::nullopt;

// used in letting lua scripts wait without blocking the rest of the program
struct WaitingCoroutine {
    sol::coroutine coroutine;
    long long framesLeft;
};
std::vector<WaitingCoroutine> YIELDED_COROUTINES;

LuaHandler& LuaHandler::Get() {
    static LuaHandler lh;
    return lh;
}

int ExceptionHandler(lua_State* L, sol::optional<const std::exception&> maybe_exception, sol::string_view description) {
    DebugLogError("Lua exception handler was called.");
    if (maybe_exception) {
        DebugLogError("While attempting to run lua file:\n", maybe_exception->what());
    }
    else {
        DebugLogError("While attempting to run lua file (specific exception unavailable):\n", description.data());
    }   

    return sol::stack::push(L, description);
}

void Wait(float time, sol::thread thread) {
    std::cout << "Wait for " << time << ".\n";
    // std::cout << "Coroutine type is " << (coroutine.get_type());
    sol::coroutine coroutine = thread.as<sol::coroutine>();
    std::cout << "conversion successful.\n";
    YIELDED_COROUTINES.push_back({coroutine, static_cast<long long>(time*60.0f)});
    std::cout << "a.\n";
}

void Require(std::string key, std::string filepath) {
    LUA_STATE->require_file(key, filepath);
}

LuaHandler::LuaHandler() {
    assert(!LUA_STATE.has_value()); // the constructor is supposed to make a lua state so there better not already be one
    
    // intitialize lua
    LUA_STATE = sol::state();

    // setup lua standard library
    LUA_STATE->open_libraries(sol::lib::base, sol::lib::os, sol::lib::math, sol::lib::table, sol::lib::coroutine, sol::lib::string);

    // say stuff when lua does sad face error
    LUA_STATE->set_exception_handler(&ExceptionHandler);
    // sol::protected_function::set_default_handler(ExceptionHandler);
    
    // expose our stuff to lua
    // engines
    // LUA_STATE->set("GE", &GraphicsEngine::Get());

    // waiting: very important
    LUA_STATE->set("__C_WAIT", &Wait);
    LUA_STATE->require_script("Wait", 
    "function Wait(time) print(\"kok\") print(\"yoielding\") coroutine.yield() print(\"okkk\?\?!\?\?!\") end return Wait");

    // requiring other files
    // notably, because sol is weird, require() returns nothing and instead sets the given variable name to whatever require() returns.
    LUA_STATE->set("require", &Require); 

    // enums
    auto enumTable = LUA_STATE->create_table();
    enumTable.new_enum<ComponentRegistry::ComponentBitIndex, true>("ComponentBitIndex", {
            {"Transform", ComponentRegistry::TransformComponentBitIndex},
            {"Render", ComponentRegistry::RenderComponentBitIndex}, 
            {"RenderNoFloatingOrigin", ComponentRegistry::RenderComponentNoFOBitIndex},
            {"Rigidbody", ComponentRegistry::RigidbodyComponentBitIndex},
            {"Collider", ComponentRegistry::ColliderComponentBitIndex},
            {"PointLight", ComponentRegistry::PointlightComponentBitIndex},
            {"AudioPlayer", ComponentRegistry::AudioPlayerComponentBitIndex}
        });
    
    LUA_STATE->set("Enum", enumTable);

    // glm types
    SetupVecUsertype<glm::vec3, float>(&*LUA_STATE, "Vec3f");
    SetupVecUsertype<glm::dvec3, double>(&*LUA_STATE, "Vec3d");

    auto quatUserType = LUA_STATE->new_usertype<glm::quat>("Quat", sol::constructors<glm::quat(glm::vec3), glm::quat(glm::quat), glm::quat(float, float, float, float)>());


    // gameobejcts and their componentts
    auto gameObjectCreateParamsUsertype = LUA_STATE->new_usertype<LuaGameobjectCreateParams>("GameObjectCreateParams", sol::constructors<LuaGameobjectCreateParams(sol::lua_table)>());
    gameObjectCreateParamsUsertype["physMeshId"] = &LuaGameobjectCreateParams::physMeshId;
    gameObjectCreateParamsUsertype["meshId"] = &LuaGameobjectCreateParams::meshId;
    gameObjectCreateParamsUsertype["materialId"] = &LuaGameobjectCreateParams::materialId;
    gameObjectCreateParamsUsertype["shaderId"] = &LuaGameobjectCreateParams::shaderId;

    auto transformComponentUsertype = LUA_STATE->new_usertype<TransformComponent>("Transform", sol::no_constructor);
    transformComponentUsertype["position"] = sol::property(&TransformComponent::Position, &TransformComponent::SetPos);
    transformComponentUsertype["rotation"] = sol::property(&TransformComponent::Rotation, &TransformComponent::SetRot);
    transformComponentUsertype["scale"] = sol::property(&TransformComponent::Scale, &TransformComponent::SetScl);
    transformComponentUsertype["parent"] = sol::property(&TransformComponent::GetParent, &TransformComponent::SetParent);
    // transformComponentUsertype["children"] = sol::readonly_property(&TransformComponent::children); TODO

    auto gameObjectUsertype = LUA_STATE->new_usertype<GameObject>("GameObject", sol::factories(LuaGameobjectConstructor));
    gameObjectUsertype["transform"] = sol::property(&GameObject::LuaGetTransform);

    // LUA_STATE->set_function("NewGameObject", ComponentRegistry::NewGameObject);
}

void LuaHandler::RunString(const std::string source) {
    
    auto result = LUA_STATE->safe_script(source, &sol::script_pass_on_error);
    if (!result.valid()) {
        DebugLogError("Lua failed to run with error ");
    }
}

void LuaHandler::RunFile(const std::string scriptPath) {
    // "safe" means it won't just throw if the script doesn't successfully run to completion
    // we need to actually wrap this call in a coroutine by requiring it 
    // TODO: would this technically allow the equivalent of SQL injection? probably
    // RunString("coroutine.resume(coroutine.create(function() require(\"__IGNORE\", \"" + scriptPath + "\") end))");
    // RunString("result, message = pcall(function() require(\"__IGNORE\", \"" + scriptPath + "\") end) if not result then error(message) end");

    RunString("local __NEW_COROUTINE = coroutine.create(function() require(\"__IGNORE\", \"" + scriptPath + "\") end)");
    sol::coroutine routine = (*LUA_STATE)["__NEW_COROUTINE"];
    routine();
    // sol::thread coroutineRunner = sol::thread::create(LUA_STATE->lua_state());
    // auto runnerView = coroutineRunner.thread_state();
    // runnerView[]

    // auto result = LUA_STATE->safe_script_file(scriptPath, &sol::script_pass_on_error);
}

LuaHandler::~LuaHandler() {
    LUA_STATE = std::nullopt; // delete all the lua stuff and let sol2 cleanup
}

void LuaHandler::OnFrameBegin() {

    // resume waiting coroutines

    std::vector<unsigned int> indicesToRemove;
    unsigned int i = 0;
    for (auto & co: YIELDED_COROUTINES) {
        co.framesLeft -= 1;
        if (co.framesLeft == 0) {
            // co.coroutine.;
            indicesToRemove.push_back(i);
        } 
        i++;
    }

    for (auto it = indicesToRemove.rbegin(); it != indicesToRemove.rend(); it++) {
        YIELDED_COROUTINES.erase(YIELDED_COROUTINES.begin() + *it);
    }
}

void LuaHandler::PreRenderCallbacks() {

}

void LuaHandler::PostRenderCallbacks() {

}

void LuaHandler::PrePhysicsCallbacks() {

}

void LuaHandler::PostPhysicsCallbacks() {

}