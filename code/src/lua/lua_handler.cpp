// #define DEBUG_LUA
// #ifdef DEBUG_LUA
// #define SOL_ALL_SAFETIES_ON 1
// #define 
// #endif

#include "debug/log.hpp"
#include "gameobjects/gameobject.hpp"
// #include "lua/lua_component_wrappers.hpp"
#ifndef SOL_ALL_SAFETIES_ON 
#error bruh
#endif

#include "lua_handler.hpp"
// #define SOL_USE_LUA_HPP
#include <sol/sol.hpp>
#include "debug/assert.hpp"
#include <optional>

#include "lua_constructor_wrappers.hpp"

#include "physics/raycast.hpp"

// TODO: ASYNCHRONOUS ASSET LOADING

// normally this stuff would be private members of the lua handler class, but then the header file would have to include all of sol2 which would probably make compile times worse
// we use sol2 for lua support.
// TODO: if we ever do dlls/modules it can't be like this
std::optional<sol::state> LUA_STATE = std::nullopt;
// sol::optional<sol::thread> LUA_THREAD = std::nullopt; // we have to run coroutines on threads

// used in letting lua scripts wait without blocking the rest of the program
// struct WaitingCoroutine {
//     sol::main_coroutine coroutine;
//     long long framesLeft;
// };
// std::vector<WaitingCoroutine> YIELDED_COROUTINES;

#ifdef IS_MODULE
LuaHandler* _LUA_HANDLER_ = nullptr;
void LuaHandler::SetModuleLuaHandler(LuaHandler* handler) {
    _LUA_HANDLER_ = handler;
}
#endif

LuaHandler& LuaHandler::Get() {
    #ifdef IS_MODULE
    Assert(_LUA_HANDLER_ != nullptr);
    return *_LUA_HANDLER_;
    #else
    static LuaHandler handler;
    return handler;
    #endif
}

// int ExceptionHandler(lua_State* L, sol::optional<const std::exception&> maybe_exception, sol::string_view description) {
//     DebugLogError("Lua exception handler was called.");
//     if (maybe_exception) {
//         DebugLogError("While attempting to run lua file:\n", maybe_exception->what());
//     }
//     else {
//         DebugLogError("While attempting to run lua file (specific exception unavailable):\n", description.data());
//     }   

//     return sol::stack::push(L, description);
// }

// Returns true if the result is valid. Else, returns false and prints error messages
bool HandleMaybeLuaError(const sol::protected_function_result& result, std::string fileNameOrSource) {
    
    if (result.valid()) {
        return true;
    }
    else {
        sol::error err = result;
        DebugLogError("While attemping to run lua \"", fileNameOrSource, "\":\n", err.what());
        return false;
    }
}

// void Wait(sol::object lTime) {
//     Assert(lTime.is<double>());
//     // LUA_STATE->set(Args &&args...)
//     double time = lTime.as<double>();

//     DebugLogInfo("Wait for ", int(time * 60.0), ".\n");
  
//     sol::coroutine co = LUA_THREAD->state()["__MAIN_CO_"];
//     // TODO: waiting needs to be better set delay
//     std::vector<std::pair<sol::coroutine, int>>& tbl = LUA_THREAD->state()["__YIELDED_CO_"];
//     tbl.push_back({co, int(time * 60.0)});
//     // DebugLogInfo("Yielding coroutine now.");
// }

#include <fstream>
sol::protected_function_result LoadFileAsFunction(std::string filepath) {
    std::fstream t;
    t.open(filepath);

    std::string buffer = "function __LOADED_FILE_AS_FUNC_ ()\n ";
    std::string line;
    while(t && std::getline(t, line)){
        buffer += line;
        buffer += "\n"; // getline() deletes the newlines, annoyingly, and then comments in lua code wipe out the whole script
    }
    t.close();

    
    buffer += "\n end return __LOADED_FILE_AS_FUNC_";

    // DebugLogInfo("Loading string into sol: \n", buffer);

    // todo: this does unneccesarily do some copies and stuff
    return LUA_STATE.value().safe_script(buffer);
}

sol::object Require(std::string key, std::string filepath) {
    return LUA_STATE->require_file(key, filepath);
}

// Fun fact: sol/lua doesn't like stuff by reference, so we have to give pointers instead for all the singletons. Doesn't effect lua side of things.
Window* GetWindow() {
    return &GraphicsEngine::Get().window;
}

Camera* GetCurrentCamera() {
    return &GraphicsEngine::Get().GetCurrentCamera();
}

GraphicsEngine* GetGE() {
    return &GraphicsEngine::Get();
}

LuaHandler::LuaHandler() {
    Assert(!LUA_STATE.has_value()); // the constructor is supposed to make a lua state so there better not already be one
    
    // intitialize lua
    LUA_STATE = sol::state();
    // LUA_THREAD = sol::thread::create(LUA_STATE->lua_state());

    // setup lua standard library
    LUA_STATE->open_libraries(sol::lib::base, sol::lib::os, sol::lib::math, sol::lib::table, sol::lib::coroutine, sol::lib::string, sol::lib::debug);

    // say stuff when lua does sad face error
    // sol::set_default_exception_handler(LUA_STATE->lua_state(), &ExceptionHandler);
    // LUA_STATE->set_exception_handler(&ExceptionHandler);
    // sol::protected_function::set_default_handler(ExceptionHandler);
    
    // camera
    auto cameraUsertype = LUA_STATE->new_usertype<Camera>("Camera", sol::constructors<Camera()>());
    cameraUsertype["position"] = &Camera::position;
    cameraUsertype["rotation"] = &Camera::rotation;
    cameraUsertype["fov"] = &Camera::fieldOfView;
    cameraUsertype["nearDistance"] = &Camera::near;
    cameraUsertype["farDistance"] = &Camera::far;
    LUA_STATE->set_function("GetCurrentCamera", &GetCurrentCamera);

    // expose our stuff to lua
    // engines
    auto GEUsertype = LUA_STATE->new_usertype<GraphicsEngine>("GraphicsEngine", sol::no_constructor);
    // GEUsertype["SetCurrentCamera"] TODO
    GEUsertype["debugFreecamEnabled"] = &GraphicsEngine::debugFreecamEnabled;

    LUA_STATE->set_function("GetGraphicsEngine", GetGE);

    // waiting/yielding: very important
    // LUA_STATE->set("__C_WAIT", sol::yielding(Wait));
    // LUA_STATE->set("__YIELDED_CO_", std::vector<std::pair<sol::coroutine, int>> {});
    // LUA_STATE->require_script("Wait", 
    // "function Wait(time) print(\"kok\") __WAIT_DURATION_ = time print(\"yoielding\") __C_WAIT(1.0) print(\"control returned to lua script\") end return Wait");

    // // requiring other files; only used internally
    // // notably, because sol is weird, require() returns nothing and instead sets the given variable name to whatever require() returns.
    LUA_STATE->set("require", &Require); 

    // // enums
    auto enumTable = LUA_STATE->create_table();
    enumTable.new_enum<ComponentBitIndex::ComponentBitIndex, true>("ComponentBitIndex", {
            {"Transform", ComponentBitIndex::Transform},
            {"Render", ComponentBitIndex::Render}, 
            {"RenderNoFloatingOrigin", ComponentBitIndex::RenderNoFO},
            {"Rigidbody", ComponentBitIndex::Rigidbody},
            {"Collider", ComponentBitIndex::Collider},
            {"PointLight", ComponentBitIndex::Pointlight},
            {"AudioPlayer", ComponentBitIndex::AudioPlayer}
        });

    enumTable.new_enum<Texture::TextureUsage, true>("TextureUsage", {
        {"ColorMap", Texture::TextureUsage::ColorMap},
        {"DisplacementMap", Texture::TextureUsage::DisplacementMap},
        {"FontMap", Texture::TextureUsage::FontMap},
        {"SpecularMap", Texture::TextureUsage::SpecularMap},
        {"NormalMap", Texture::TextureUsage::NormalMap},
    });
    enumTable.new_enum<Texture::TextureFilteringBehaviour, true>("TextureFilteringBehaviour", {
        {"LinearFiltering", Texture::TextureFilteringBehaviour::LinearTextureFiltering},
        {"NoFiltering", Texture::TextureFilteringBehaviour::NoTextureFiltering}
    });
    enumTable.new_enum<Texture::TextureMipmapBehaviour, true>("TextureMipmapBehaviour", {
        {"Linear", Texture::TextureMipmapBehaviour::LinearMipmapInterpolation},
        {"NearestMipmap", Texture::TextureMipmapBehaviour::UseNearestMipmap},
        {"Off", Texture::TextureMipmapBehaviour::NoMipmaps}
    });
    enumTable.new_enum<Texture::TextureType, true>("TextureType", {
        {"Texture2D", Texture::TextureType::Texture2D},
        {"TextureCubemap", Texture::TextureType::TextureCubemap}
    });
    enumTable.new_enum<int, true>("InputType", {
        {"A", GLFW_KEY_A},
        {"B", GLFW_KEY_B},
        {"C", GLFW_KEY_C},
        {"D", GLFW_KEY_D},
        {"E", GLFW_KEY_E},
        {"F", GLFW_KEY_F},
        {"G", GLFW_KEY_G},
        {"H", GLFW_KEY_H},
        {"I", GLFW_KEY_I},
        {"J", GLFW_KEY_J},
        {"K", GLFW_KEY_K},
        {"L", GLFW_KEY_L},
        {"M", GLFW_KEY_M},
        {"N", GLFW_KEY_N},
        {"O", GLFW_KEY_O},
        {"P", GLFW_KEY_P},
        {"Q", GLFW_KEY_Q},
        {"R", GLFW_KEY_R},
        {"S", GLFW_KEY_S},
        {"T", GLFW_KEY_T},
        {"U", GLFW_KEY_U},
        {"V", GLFW_KEY_V},
        {"W", GLFW_KEY_W},
        {"X", GLFW_KEY_X},
        {"Y", GLFW_KEY_Y},
        {"Z", GLFW_KEY_Z},

        {"Space", GLFW_KEY_SPACE},
        {"Escape", GLFW_KEY_ESCAPE},
    });

    LUA_STATE->set("Enum", enumTable);
    // (*LUA_STATE)["__YIELDED_CO_"] = LUA_STATE->create_table();
    // sol::table tbl = (*LUA_STATE)["__YIELDED_CO_"] ;

    // glm types
    SetupVec3Usertype<glm::vec3, float>(&*LUA_STATE, "Vec3f");
    SetupVec3Usertype<glm::dvec3, double>(&*LUA_STATE, "Vec3d");
    SetupVec4Usertype<glm::vec4, float>(&*LUA_STATE, "Vec4f");
    SetupVec2Usertype<glm::vec2, float>(&*LUA_STATE, "Vec2f");

    auto quatUserType = LUA_STATE->new_usertype<glm::quat>("Quat", sol::constructors<glm::quat(glm::vec3), glm::quat(glm::quat), glm::quat(float, float, float, float)>());

    auto mat3x3Usertype = LUA_STATE->new_usertype<glm::mat3x3>("Mat3x3", sol::factories(glm::identity<glm::mat3x3>));    

    // windowing/input
    LUA_STATE->set_function("GetWindow", GetWindow);
    auto windowUsertype = LUA_STATE->new_usertype<Window>("Window", sol::no_constructor);
    windowUsertype["width"] = &Window::width;
    windowUsertype["height"] = &Window::height;
    windowUsertype["mouseLocked"] = sol::property(&Window::SetMouseLocked);
    windowUsertype["Close"] = &Window::Close;
    windowUsertype["ShouldClose"] = &Window::ShouldClose;
    /*windowUsertype["IsPressBegan"] = &Window::IsPressBegan;
    windowUsertype["IsPressEnded"] = &Window::IsPressEnded;
    windowUsertype["IsPressed"] = &Window::IsPressed;*/

    // meshes
    auto meshCreateParamsUsertype = LUA_STATE->new_usertype<MeshCreateParams>("MeshCreateParams", sol::factories(MeshCreateParams::Default));
    //meshCreateParamsUsertype["dynamic"] = &MeshCreateParams::dynamic;
    meshCreateParamsUsertype["textureZ"] = &MeshCreateParams::textureZ;
    meshCreateParamsUsertype["opacity"] = &MeshCreateParams::opacity;
    meshCreateParamsUsertype["expectedCount"] = &MeshCreateParams::expectedCount;
    meshCreateParamsUsertype["normalizeSize"] = &MeshCreateParams::normalizeSize;
    // meshCreateParamsUsertype["meshVertexFormat"] = &MeshCreateParams::meshVertexFormat; // TODO

    auto meshUsertype = LUA_STATE->new_usertype<Mesh>("Mesh", sol::factories(LuaMeshConstructor));
    meshUsertype["id"] = &Mesh::meshId;
    meshUsertype["Unload"] = &Mesh::Unload;
    meshUsertype["Get"] = &Mesh::Get;

    // textures/materials
    auto textureCreateParamsUsertype = LUA_STATE->new_usertype<TextureCreateParams>("TextureCreateParams", sol::factories(LuaTextureCreateParamsConstructor));
    textureCreateParamsUsertype["format"] = &TextureCreateParams::format;
    textureCreateParamsUsertype["filteringBehaviour"] = &TextureCreateParams::filteringBehaviour;
    textureCreateParamsUsertype["mipmapBehaviour"] = &TextureCreateParams::mipmapBehaviour;

    //auto materialUsertype = LUA_STATE->new_usertype<Material>("Material", sol::factories(LuaMaterialConstructor));
    //materialUsertype["id"] = &Material::id;

    // sound
    auto soundUsertype = LUA_STATE->new_usertype<Sound>("Sound", sol::factories(&Sound::New));

    // gameobjects and their componentts
    auto gameObjectCreateParamsUsertype = LUA_STATE->new_usertype<LuaGameobjectCreateParams>("GameObjectCreateParams", sol::constructors<LuaGameobjectCreateParams(sol::lua_table)>());
    gameObjectCreateParamsUsertype["physMesh"] = &LuaGameobjectCreateParams::physMesh;
    gameObjectCreateParamsUsertype["meshId"] = &LuaGameobjectCreateParams::meshId;
    gameObjectCreateParamsUsertype["materialId"] = &LuaGameobjectCreateParams::materialId;
    gameObjectCreateParamsUsertype["sound"] = &LuaGameobjectCreateParams::sound;

    auto transformComponentUsertype = LUA_STATE->new_usertype<TransformComponent>("Transform", sol::no_constructor);
    transformComponentUsertype["position"] = sol::property(&TransformComponent::Position, &TransformComponent::SetPos);
    transformComponentUsertype["rotation"] = sol::property(&TransformComponent::Rotation, &TransformComponent::SetRot);
    transformComponentUsertype["scale"] = sol::property(&TransformComponent::Scale, &TransformComponent::SetScl);
    transformComponentUsertype["parent"] = sol::property(&TransformComponent::GetParent, &TransformComponent::SetParent);
    // transformComponentUsertype["children"] = sol::readonly_property(&TransformComponent::children); TODO

    auto renderComponentUsertype = LUA_STATE->new_usertype<RenderComponent>("Render", sol::no_constructor);
    renderComponentUsertype["color"] = sol::property(&RenderComponent::SetColor);
    renderComponentUsertype["textureZ"] = sol::property(&RenderComponent::SetTextureZ);

    auto colliderComponentUsertype = LUA_STATE->new_usertype<ColliderComponent>("Collider", sol::no_constructor);
    // colliderComponentUsertype["IsCollidingWith"] = &ColliderComponent::IsCollidingWith;
    // colliderComponentUsertype["GetColliding"] = &ColliderComponent::GetColliding;
    colliderComponentUsertype["gameObject"] = sol::property(&ColliderComponent::GetGameObject);

    auto rigidbodyComponentUsertype = LUA_STATE->new_usertype<RigidbodyComponent>("Rigidbody", sol::no_constructor);
    rigidbodyComponentUsertype["kinematic"] = &RigidbodyComponent::kinematic;
    rigidbodyComponentUsertype["velocity"] = &RigidbodyComponent::velocity;
    rigidbodyComponentUsertype["angularVelocity"] = &RigidbodyComponent::angularVelocity;
    rigidbodyComponentUsertype["mass"] = sol::property(&RigidbodyComponent::SetMass);
    rigidbodyComponentUsertype["localMomentOfInertia"] = &RigidbodyComponent::localMomentOfInertia;
    rigidbodyComponentUsertype["linearDrag"] = &RigidbodyComponent::linearDrag;
    rigidbodyComponentUsertype["angularDrag"] = &RigidbodyComponent::angularDrag;

    auto pointlightComponentUsertype = LUA_STATE->new_usertype<PointLightComponent>("PointLight", sol::no_constructor);
    pointlightComponentUsertype["range"] = sol::property(&PointLightComponent::Range, &PointLightComponent::SetRange);
    pointlightComponentUsertype["color"] = sol::property(&PointLightComponent::Color, &PointLightComponent::SetColor);

    auto audioPlayerComponentUsertype = LUA_STATE->new_usertype<AudioPlayerComponent>("AudioPlayer", sol::no_constructor);
    audioPlayerComponentUsertype["IsPlaying"] = &AudioPlayerComponent::IsPlaying;
    audioPlayerComponentUsertype["Resume"] = &AudioPlayerComponent::Resume;
    audioPlayerComponentUsertype["Stop"] = &AudioPlayerComponent::Stop;
    audioPlayerComponentUsertype["Pause"] = &AudioPlayerComponent::Pause;
    audioPlayerComponentUsertype["Play"] = &AudioPlayerComponent::Play;
    
    audioPlayerComponentUsertype["sound"] = sol::property(&AudioPlayerComponent::SetSound);
    audioPlayerComponentUsertype["doppler"] = &AudioPlayerComponent::doppler;
    audioPlayerComponentUsertype["pitch"] = &AudioPlayerComponent::pitch;
    audioPlayerComponentUsertype["volume"] = &AudioPlayerComponent::volume;
    audioPlayerComponentUsertype["rolloff"] = &AudioPlayerComponent::rolloff;
    audioPlayerComponentUsertype["looped"] = &AudioPlayerComponent::looped;
    audioPlayerComponentUsertype["positional"] = &AudioPlayerComponent::positional;

    auto gameObjectUsertype = LUA_STATE->new_usertype<GameObject>("GameObject", sol::factories(LuaGameobjectConstructor));
    //gameObjectUsertype["transform"] = sol::property(&GameObject::LuaGetTransform);
    //gameObjectUsertype["render"] = sol::property(&GameObject::LuaGetRender);
    //gameObjectUsertype["rigidbody"] = sol::property(&GameObject::LuaGetRigidbody);
    //gameObjectUsertype["collider"] = sol::property(&GameObject::LuaGetCollider);
    //gameObjectUsertype["pointLight"] = sol::property(&GameObject::LuaGetPointLight);
    //gameObjectUsertype["audioPlayer"] = sol::property(&GameObject::LuaGetAudioPlayer);

    gameObjectUsertype["Destroy"] = &GameObject::Destroy;

    // raycast
    auto raycastResultUsertype = LUA_STATE->new_usertype<RaycastResult>(sol::no_constructor);
    raycastResultUsertype["hitObject"] = sol::readonly(&RaycastResult::hitObject);
    raycastResultUsertype["hitNormal"] = sol::readonly(&RaycastResult::hitNormal);
    raycastResultUsertype["hitPoint"] = sol::readonly(&RaycastResult::hitPoint);
    LUA_STATE->set_function("Raycast", &Raycast);

    // LUA_STATE->set_function("NewGameObject", ComponentRegistry::NewGameObject);

    HandleMaybeLuaError(LUA_STATE.value().safe_script_file("../scripts/scheduler.lua"), "../scripts/scheduler.lua");
}

void LuaHandler::RunString(const std::string source) {
    
    auto result = LUA_STATE->safe_script(source, &sol::script_pass_on_error);
    HandleMaybeLuaError(result, source);
}

unsigned int lastCoroutineId = 0;

// void RunLuaCoroutine(sol::main_coroutine& co, const std::string& scriptPath) {
//     auto result = co();

        
//     DebugLogInfo("RunFile: We have been blessed with a coroutine ", co.pointer());

//     if (HandleMaybeLuaError(result, scriptPath) && co.runnable()) {
//         sol::object wait_duration = (*LUA_STATE)["__WAIT_DURATION_"];
//         double wait_time = 0;
//         if (!wait_duration.is<double>()) {
//             wait_time = wait_duration.as<double>();
//             LUA_STATE->set("__WAIT_DURATION_", sol::nil);
//         }   

//         DebugLogInfo("Setting coroutine.");
//         sol::table tbl = LUA_STATE->create_table();
//         unsigned int id = lastCoroutineId++;
//         tbl["frames"] = wait_time * 60.0f;
//         tbl["coroutine"] = co;
//         (*LUA_STATE)["__YIELDED_CO_"][id] = tbl; 
//         // YIELDED_COROUTINES.push_back({co, static_cast<long long>(wait_time * 60.0f)});
//     }
// }

void LuaHandler::RunFile(const std::string scriptPath) {
    // "safe" means it won't just throw if the script doesn't successfully run to completion
    // we need to actually wrap whatever the file is in a function call, which is what LoadFileAsFunction does, so that it can be called with a coroutine so it can wait and stuff.
    auto loadedFunction = LoadFileAsFunction(scriptPath);
    if (HandleMaybeLuaError(loadedFunction, scriptPath)) {
        HandleMaybeLuaError(LUA_STATE.value()["DoTask"](sol::function(loadedFunction), scriptPath), scriptPath);
    }
    
}

LuaHandler::~LuaHandler() {
    LUA_STATE = std::nullopt; // delete all the lua stuff and let sol2 cleanup
}

void LuaHandler::OnFrameBegin() {

    HandleMaybeLuaError(LUA_STATE.value()["ResumeTasks"](), "???");

    // resume waiting coroutines

    // std::vector<unsigned int> indicesToRemove;
    // unsigned int i = 0;
    // // // for (auto & co: YIELDED_COROUTINES) {
    // // DebugLogInfo("test");
    // auto & tbl = LUA_THREAD->state().get<std::vector<std::pair<sol::coroutine, int>>&>("__YIELDED_CO_");
    // auto & tbl_copy = tbl;
    // // DebugLogInfo("Size of tbl = ", tbl.size());

    // // std::vector<sol::object> keysToErase;

    // for (auto & [co, time] : tbl_copy) {
    //     DebugLogInfo("Time = ", time);
    //     time -= 1;
    //     if (time <= 0) {
    //         Assert(co.runnable());
    //         // sol::coroutine co = pair.second.as<sol::coroutine>();
    //         // keysToErase.push_back(pair.first);
    //         co();
    //         if (!co.runnable()) {
    //             indicesToRemove.push_back(i);
    //             i++;
    //         }
            
    //     }
        

    //     // break;

    //     // sol::table co = pair.second.as<sol::table>();
    //     // sol::object framesLeft = co["frames"];
    //     // co["frames"] = framesLeft.as<double>() - 1;
    //     // if (framesLeft <= 0) {
    //     //     // Assert(co.coroutine.runnable());
    //     //     DebugLogInfo("Getting pointer.");
    //     //     DebugLogInfo("Running coroutine at ", co.coroutine.pointer());
    //     //     while (co.coroutine.runnable()) {
    //     //         // DebugLogInfo("run?");
    //     //         RunLuaCoroutine(co.coroutine, std::string("??? on frame begin ???"));
    //     //     }
            
    //     // } 
        
    // }

    // for (auto & key: keysToErase) {
    //     tbl[key] = sol::nil;
    // }

    // for (auto it = indicesToRemove.rbegin(); it != indicesToRemove.rend(); it++) {
    //     tbl.erase(tbl.begin() + *it);
    // }
}

void LuaHandler::PreRenderCallbacks() {

}

void LuaHandler::PostRenderCallbacks() {

}

void LuaHandler::PrePhysicsCallbacks() {

}

void LuaHandler::PostPhysicsCallbacks() {

}