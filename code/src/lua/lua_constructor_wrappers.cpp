#include "lua_constructor_wrappers.hpp"
#include "graphics/mesh.hpp"
#include "graphics/texture.hpp"
#include <string>
#include <tuple>
#include <vector>

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
        sol::error("Gameobject.new() takes a GameobjectCreateParams as an argument, nothing else.");
    }
    auto params = args.as<LuaGameobjectCreateParams>();
    if (!Mesh::IsValidForGameObject(params.meshId)) {
        throw sol::error("Invalid mesh id given. Please use a real mesh that can actually be rendered.");
    }
    return ComponentRegistry::Get().NewGameObject(params);
}

sol::variadic_results LuaMeshConstructor(sol::object arg1, sol::object arg2) {
    if (arg1.is<std::string>() && (arg2 == sol::nil || arg2.is<MeshCreateParams>())) {
        auto result = (arg2 == sol::nil) ? Mesh::MultiFromFile(arg1.as<std::string>()) : Mesh::MultiFromFile(arg1.as<std::string>(), arg2.as<MeshCreateParams>());
        sol::variadic_results luaifiedResults;
        for (auto & [a, b, c, d]: result) {
            sol::table tabl;
            tabl["mesh"] = a;
            tabl["material"] = b;
            tabl["textureZ"] = c;
            tabl["offset"] = d;
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
        std::vector<std::string> filepathArray;
        for (auto & [key, value] : filepath.as<sol::table>()) {
            if (!value.is<std::string>()) {
                throw sol::error("The 1st argument to TextureCreateParams.new() should be a filepath string or a table of filepath strings. You passed a table, but there's something in it that is not a string. Fix it.");
            }
            filepathArray.push_back(filepath.as<std::string>());
        }
        return TextureCreateParams(filepathArray, textureUsage.as<Texture::TextureUsage>());
    }
    else {
        throw sol::error("The 1st argument to TextureCreateParams.new() should be a filepath string or a table of filepath strings.");
    } 
}

std::tuple<std::shared_ptr<Material>, float> LuaMaterialConstructor(sol::variadic_args args) {
    if (args.size() < 2 || !args[0].is<Texture::TextureType>()) {
        throw sol::error("Material.new() expects at least two arguments, the first of which should be a TextureType enum and all following arguments should be instances of TextureCreateParams.");
    }

    unsigned int i = 1; // we start iterating from 2nd arg because those are the texture create params
    std::vector<TextureCreateParams> params;
    for (auto it = args.begin() + 1; it != args.end(); it++) {
        auto obj = *it;
        if (!obj.is<TextureCreateParams>()) {
            throw sol::error(std::string("Argument at index ") + std::to_string(i) + " to Material.new() is not a TextureCreateParams. Please fix that.");
        }
        else {
            params.push_back(obj);
        }
        i++;
    }

    auto [textureZ, material] = Material::New(params, args[0]);
    return std::make_tuple(material, textureZ);
}