#include "material.hpp"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>

void Material::Destroy(const unsigned int id) {
    assert(MATERIALS.count(id));
    MATERIALS.erase(id);
}

std::shared_ptr<Material>& Material::Get(unsigned int materialId) {
    assert(MATERIALS.count(materialId));
    return MATERIALS.at(materialId);
}

// TODO: needs to try to append to existing material
std::pair<float, std::shared_ptr<Material>> Material::New(const std::vector<TextureCreateParams>& textureParams, TextureType type) {
    auto ptr = std::shared_ptr<Material>(new Material(textureParams, type));
    MATERIALS[ptr->id] = ptr;
    return std::make_pair(0, ptr);
}

Material::Material(const std::vector<TextureCreateParams>& textureParams, TextureType type): 
id(LAST_MATERIAL_ID++),
materialType(type),
colorMap(std::nullopt),
normalMap(std::nullopt),
specularMap(std::nullopt)
{
    // Just go through textureParams and create each texture they ask for
    for (auto & textureToMake: textureParams) {
        switch (textureToMake.usage) {
            case ColorMap:
            if (colorMap) { // Make sure bro isn't trying to create a material with two different textures for color map
                std::cout << "PROBLEM: you tried to give us two different color textures for this material. Thats bad.\n";
                abort();
            }
            colorMap.emplace(textureToMake, COLORMAP_TEXTURE_INDEX, type); // constructs the texture inside the optional without copying
            break;
            case NormalMap:
            if (normalMap) { // Make sure bro isn't trying to create a material with two different textures for normal map
                std::cout << "PROBLEM: you tried to give us two different normal textures for this material. Thats bad.\n";
                abort();
            }
            normalMap.emplace(textureToMake, NORMALMAP_TEXTURE_INDEX, type); // constructs the texture inside the optional without copying
            break;
            case SpecularMap:
            if (specularMap) { // Make sure bro isn't trying to create a material with two different textures for specular map
                std::cout << "PROBLEM: you tried to give us two different specular textures for this material. Thats bad.\n";
                abort();
            }
            specularMap.emplace(textureToMake, SPECULARMAP_TEXTURE_INDEX, type); // constructs the texture inside the optional without copying
            break;
            case DisplacementMap:
            if (displacementMap) { // Make sure bro isn't trying to create a material with two different textures for displacement map
                std::cout << "PROBLEM: you tried to give us two different displacement textures for this material. Thats bad.\n";
                abort();
            }
            displacementMap.emplace(textureToMake, DISPLACEMENTMAP_TEXTURE_INDEX, type); // constructs the texture inside the optional without copying
            break;
            default:
            std::cout << "material constructor: what are you doing???\n";
            abort();
            break;
        }
    }
}

void Material::Use() {
    colorMap->Use();
    if (specularMap) specularMap->Use();
    if (normalMap) normalMap->Use();
    if (displacementMap) displacementMap->Use();
}

// TODO: this feels bad.
void Material::Unbind() {
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