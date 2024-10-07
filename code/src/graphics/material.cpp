#include "material.hpp"
#include "graphics/mesh.hpp"
#include "debug/assert.hpp"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <process.h>
#include "shader_program.hpp"

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
textures(std::nullopt)
{
    // Just go through textureParams and create each texture they ask for
    // TODO: lots of sanity checks should be made here on the params for each texture
    unsigned int i = 0;
    for (auto & textureToMake: params.textureParams) {

        Assert(textures->Count(textureToMake.usage) == 0);

        switch (textureToMake.usage) {
            case Texture::ColorMap:
            textures->textures.at(i).emplace(textureToMake, COLORMAP_TEXTURE_INDEX, params.type); // constructs the texture inside the optional without copying
            break;
            case Texture::NormalMap:
            textures->textures.at(i).emplace(textureToMake, NORMALMAP_TEXTURE_INDEX, params.type); // constructs the texture inside the optional without copying
            break;
            case Texture::SpecularMap:
            textures->textures.at(i).emplace(textureToMake, SPECULARMAP_TEXTURE_INDEX, params.type); // constructs the texture inside the optional without copying
            break;
            case Texture::DisplacementMap:
            textures->textures.at(i).emplace(textureToMake, DISPLACEMENTMAP_TEXTURE_INDEX, params.type); // constructs the texture inside the optional without copying
            break;
            case Texture::FontMap:
            textures->textures.at(i).emplace(textureToMake, FONTMAP_TEXTURE_INDEX, params.type); // constructs the texture inside the optional without copying
            break;
            default:
            std::cout << "material constructor: what are you doing???\n";
            abort();
            break;
        }

        i++;
    }
}

void Material::Use(std::shared_ptr<ShaderProgram> currentShader) {
    for (auto& t : textures->textures) {
        if (t.has_value()) {
            t->Use();
        }
    }

    inputProvider.onBindingFunc(this, currentShader);

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

unsigned int Material::TextureCollection::Count(Texture::TextureUsage texUsage)
{
    unsigned int count = 0;
    for (auto& t : textures) {
        if (t.has_value() && t->usage == texUsage) {
            count++;
        }
    }
    return count;
}
