#include "material.hpp"
#include "graphics/mesh.hpp"
#include "debug/assert.hpp"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <process.h>

void Material::Destroy(const unsigned int id) {
    Assert(MeshGlobals::Get().MATERIALS.count(id));
    MeshGlobals::Get().MATERIALS.erase(id);
}

std::shared_ptr<Material>& Material::Get(unsigned int materialId) {
    Assert(MeshGlobals::Get().MATERIALS.count(materialId));
    return MeshGlobals::Get().MATERIALS.at(materialId);
}

// TODO: needs to try to append to existing material
std::pair<float, std::shared_ptr<Material>> Material::New(const MaterialCreateParams& params) {
    auto ptr = std::shared_ptr<Material>(new Material(params));
    MeshGlobals::Get().MATERIALS[ptr->id] = ptr;
    return std::make_pair(0, ptr);
}

const std::shared_ptr<Material::TextureCollection>& Material::GetTextureCollection() const
{
    return textures;
}

std::optional<float> Material::TryAppendLayer(const MaterialCreateParams& params)
{
    if (params.depthMask != depthMaskEnabled || materialType != params.type) {
        return std::nullopt;
    }
}

Material::Material(const MaterialCreateParams& params):
id(MeshGlobals::Get().LAST_MATERIAL_ID++),
materialType(params.type),
depthMaskEnabled(params.depthMask),
colorMap(std::nullopt),
normalMap(std::nullopt),
specularMap(std::nullopt),
displacementMap(std::nullopt),
fontMap(std::nullopt)
{
    // Just go through textureParams and create each texture they ask for
    // TODO: lots of sanity checks should be made here on the params for each texture
    for (auto & textureToMake: params.textureParams) {
        switch (textureToMake.usage) {
            case Texture::ColorMap:
            if (colorMap) { // Make sure bro isn't trying to create a material with two different textures for color map
                std::cout << "PROBLEM: you tried to give us two different color textures for this material. Thats bad.\n";
                abort();
            }
            colorMap.emplace(textureToMake, COLORMAP_TEXTURE_INDEX, params.type); // constructs the texture inside the optional without copying
            break;
            case Texture::NormalMap:
            if (normalMap) { // Make sure bro isn't trying to create a material with two different textures for normal map
                std::cout << "PROBLEM: you tried to give us two different normal textures for this material. Thats bad.\n";
                abort();
            }
            normalMap.emplace(textureToMake, NORMALMAP_TEXTURE_INDEX, params.type); // constructs the texture inside the optional without copying
            break;
            case Texture::SpecularMap:
            if (specularMap) { // Make sure bro isn't trying to create a material with two different textures for specular map
                std::cout << "PROBLEM: you tried to give us two different specular textures for this material. Thats bad.\n";
                abort();
            }
            specularMap.emplace(textureToMake, SPECULARMAP_TEXTURE_INDEX, params.type); // constructs the texture inside the optional without copying
            break;
            case Texture::DisplacementMap:
            if (displacementMap) { // Make sure bro isn't trying to create a material with two different textures for displacement map
                std::cout << "PROBLEM: you tried to give us two different displacement textures for this material. Thats bad.\n";
                abort();
            }
            displacementMap.emplace(textureToMake, DISPLACEMENTMAP_TEXTURE_INDEX, params.type); // constructs the texture inside the optional without copying
            break;
            case Texture::FontMap:
            if (fontMap) { // only one fontmap per material
                std::cout << "PROBLEM: you tried to give us two different font textures for this material. Thats bad.\n";
                abort();
            }
            fontMap.emplace(textureToMake, FONTMAP_TEXTURE_INDEX, params.type);
            break;
            default:
            std::cout << "material constructor: what are you doing???\n";
            abort();
            break;
        }
    }
}

void Material::Use() {
    if (colorMap) colorMap->Use();
    if (specularMap) specularMap->Use();
    if (normalMap) normalMap->Use();
    if (displacementMap) displacementMap->Use();
    if (fontMap) fontMap->Use();

    glDepthMask(depthMaskEnabled);
}

// TODO: this is bad.
void Material::Unbind() {
    glDepthMask(GL_TRUE);

    glActiveTexture(GL_TEXTURE0 + COLORMAP_TEXTURE_INDEX);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glBindTexture(GL_TEXTURE_3D, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glActiveTexture(GL_TEXTURE0 + NORMALMAP_TEXTURE_INDEX);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glBindTexture(GL_TEXTURE_3D, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glActiveTexture(GL_TEXTURE0 + DISPLACEMENTMAP_TEXTURE_INDEX);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glBindTexture(GL_TEXTURE_3D, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glActiveTexture(GL_TEXTURE0 + SPECULARMAP_TEXTURE_INDEX);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glBindTexture(GL_TEXTURE_3D, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glActiveTexture(GL_TEXTURE0 + FONTMAP_TEXTURE_INDEX);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glBindTexture(GL_TEXTURE_3D, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

bool Material::HasNormalMap() {
    return normalMap.has_value();
}

bool Material::HasSpecularMap() {
    return specularMap.has_value();
}

bool Material::HasDisplacementMap() {
    return displacementMap.has_value();
}

bool Material::HasFontMap() {
    return fontMap.has_value();
}

bool Material::HasColorMap() {
    return colorMap.has_value();
}