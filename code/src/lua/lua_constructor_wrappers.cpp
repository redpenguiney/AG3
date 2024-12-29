#include "lua_constructor_wrappers.hpp"
#include "graphics/mesh.hpp"
#include "graphics/texture.hpp"
#include <string>
#include <tuple>
#include <vector>

std::vector<ComponentBitIndex::ComponentBitIndex> GetComponents(sol::lua_table args) {
    std::vector<ComponentBitIndex::ComponentBitIndex> components;
    for (const auto & keyValuePair: args) {
        sol::object component = keyValuePair.second;
        if (!component.is<ComponentBitIndex::ComponentBitIndex>()) {
            throw sol::error("LuaGenGameobjectCreateParams takes an array of ComponentBitIndex as an argument. You gave it something else. Please don't.");
        }
        components.push_back(component.as<ComponentBitIndex::ComponentBitIndex>());
    }
    return components;
}

LuaGameobjectCreateParams::LuaGameobjectCreateParams(sol::lua_table args): GameobjectCreateParams(GetComponents(args)) {}

std::shared_ptr<GameObject> LuaGameobjectConstructor(sol::object args) {
    if (!args.is<LuaGameobjectCreateParams>()) {
        throw sol::error("Gameobject.new() takes a GameobjectCreateParams as an argument, nothing else.");
    }
    auto params = args.as<LuaGameobjectCreateParams>();
    if (!Mesh::IsValidForGameObject(params.meshId)) {
        throw sol::error("Invalid mesh id given. Please use a real mesh that can actually be rendered.");
    }
    return GameObject::New(params);
}

sol::variadic_results LuaMeshConstructor(sol::object arg1, sol::object arg2) {
    if (arg1.is<std::string>() && (arg2 == sol::nil || arg2.is<MeshCreateParams>())) {
        auto result = (arg2 == sol::nil) ? Mesh::MultiFromFile(arg1.as<std::string>()) : Mesh::MultiFromFile(arg1.as<std::string>(), arg2.as<MeshCreateParams>());
        sol::variadic_results luaifiedResults;
        for (auto & ret: result) {
            sol::table tabl;
            tabl["mesh"] = ret.mesh;
            tabl["material"] = ret.material;
            tabl["materialZ"] = ret.materialZ;
            tabl["posOffset"] = ret.posOffset;
            tabl["rotOffset"] = ret.rotOffset;
            luaifiedResults.push_back(tabl);
        }
        return luaifiedResults;
    }
    else {
        throw sol::error("Mesh.new() takes a path to a file and an optional MeshCreateParams, nothing else. TODO it should actually also support other things but doesn't lol");
    }
}

TextureCreateParams LuaTextureCreateParamsConstructor(sol::object filepath, sol::object textureUsage) {
    if (!textureUsage.is<Texture::TextureUsage>()) {
        throw sol::error("The 2nd argument to TextureCreateParams.new() should be a TextureUsage enum value.");
    }
    if (filepath.is<std::string>()) {
        return TextureCreateParams({filepath.as<std::string>(),}, textureUsage.as<Texture::TextureUsage>());
    }
    else if (filepath.is<sol::table>()) {
        std::vector<TextureSource> filepathArray;
        for (auto & [key, value] : filepath.as<sol::table>()) {
            if (!value.is<std::string>()) {
                throw sol::error("The 1st argument to TextureCreateParams.new() should be a filepath string or a table of filepath strings. You passed a table, but there's something in it that is not a string. Fix it.");
            }
            filepathArray.emplace_back(filepath.as<std::string>());
        }
        return TextureCreateParams(filepathArray, textureUsage.as<Texture::TextureUsage>());
    }
    else {
        throw sol::error("The 1st argument to TextureCreateParams.new() should be a filepath string or a table of filepath strings.");
    } 
}

std::tuple<std::shared_ptr<Material>, float> LuaMaterialConstructor(sol::object params) {
    if (!params.is<MaterialCreateParams>()) {
        throw sol::error("Material.new() expects a single MaterialCreateParams argument.");
    }

    auto [textureZ, material] = Material::New(params.as<MaterialCreateParams>());
    return std::make_tuple(material, textureZ);
}