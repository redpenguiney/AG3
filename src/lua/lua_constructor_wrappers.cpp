#include "lua_constructor_wrappers.hpp"
#include "graphics/mesh.hpp"

std::vector<ComponentRegistry::ComponentBitIndex> GetComponents(sol::lua_table args) {
    std::vector<ComponentRegistry::ComponentBitIndex> components;
    for (const auto & keyValuePair: args) {
        sol::object component = keyValuePair.second;
        if (!component.is<ComponentRegistry::ComponentBitIndex>()) {
            throw sol::error("LuaGenGameobjectCreateParams takes an array of ComponentBitIndex as an argument. You gave it something else. Please don't.");
        }
        components.push_back(component.as<ComponentRegistry::ComponentBitIndex>());
    }
    return components;
}

LuaGameobjectCreateParams::LuaGameobjectCreateParams(sol::lua_table args): GameobjectCreateParams(GetComponents(args)) {}

std::shared_ptr<GameObject> LuaGameobjectConstructor(sol::object args) {
    if (!args.is<LuaGameobjectCreateParams>()) {
        sol::error("Gameobject.New() takes a GameobjectCreateParams as an argument, nothing else.");
    }
    auto params = args.as<LuaGameobjectCreateParams>();
    if (!Mesh::IsValidForGameObject(params.meshId)) {
        throw sol::error("Invalid mesh id given. Please use a real mesh that can actually be rendered.");
    }
    return ComponentRegistry::Get().NewGameObject(params);
}

std::shared_ptr<Mesh> LuaMeshConstructor(sol::object arg1, sol::object arg2) {
    if (arg1.is<std::string>()) {
        
    }

    throw sol::error("Mesh::New() takes a path to a file and an optional MeshCreateParams, nothing else. TODO it should actually also support other things but doesn't lol");
}