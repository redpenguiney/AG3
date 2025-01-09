#include "material.hpp"
#include "graphics/mesh.hpp"
#include "debug/assert.hpp"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <process.h>
#include "shader_program.hpp"
#include "gengine.hpp"

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
    return std::make_pair(ptr->baseTextureZ, ptr);
}

unsigned int Material::Count(Texture::TextureUsage texUsage) const
{
    unsigned int count = 0;
    for (auto& t : textures->textures) {
        if (t.has_value() && t->usage == texUsage) {
            count++;
        }
    }
    return count;
}

const Texture& Material::Get(Texture::TextureUsage texUsage) const
{
    Assert(Count(texUsage) == 1);
    for (auto& t : textures->textures) {
        if (t.has_value() && t->usage == texUsage) {
            return *t;
        }
    }
}

//const std::shared_ptr<TextureCollection>& Material::GetTextureCollection() const
//{
//    return textures;
//}

//std::optional<float> Material::TryAppendLayer(const MaterialCreateParams& params)
//{
//    if (params.depthMask != depthMaskEnabled || materialType != params.type) {
//        return std::nullopt;
//    }
//}

Material::Material(const MaterialCreateParams& params):
id(MeshGlobals::Get().LAST_MATERIAL_ID++),
shader(params.shader ? params.shader : GraphicsEngine::Get().defaultMaterial->shader),
depthMaskEnabled(params.depthMask),
textures(params.srcTextures), 
inputProvider(params.inputProvider),
depthTestFunc(params.depthTestFunc),
blendingEnabled(params.blendingEnabled),
blendingSrcFactor(params.blendingSrcFactor),
blendingDstFactor(params.blendingDstFactor),
drawOrder(params.drawOrder)
{
    Assert(params.blendingSrcFactor.size() == params.blendingDstFactor.size());

    if (!textures) {
        auto [a, b] = TextureCollection::FindCollection(params);
        textures = a;
        baseTextureZ = b;
    }
    // there might be legitimate cases where you have a textureless material
    //Assert(params.textureParams.size() > 0);

    for (auto& textureToMake : params.textureParams)
        Assert(Count(textureToMake.usage) == 1);

    Assert(shader != nullptr);
}

// TODO: static variables that prevent redundant setting of gl state for stuff like blending and depth test/mask
void Material::Use(std::shared_ptr<ShaderProgram> currentShader) {
    for (auto& t : textures->textures) {
        if (t.has_value()) {
            t->Use();
        }
    }

    inputProvider.onBindingFunc(this, currentShader);

    if (scissoringEnabled) {
        glEnable(GL_SCISSOR_TEST);
        glScissor(scissorCorner1.x, scissorCorner1.y, scissorCorner2.x - scissorCorner1.x, scissorCorner2.y - scissorCorner2.x);
    }
    else {
        glDisable(GL_SCISSOR_TEST);
    }

    glDepthMask(depthMaskEnabled);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc((GLenum)depthTestFunc);
    if (blendingEnabled) {
        glEnable(GL_BLEND);
        for (int i = 0; i < blendingSrcFactor.size(); i++)
            glBlendFunci(i, (GLenum)blendingSrcFactor[i], (GLenum)blendingDstFactor[i]);
    }
    else {
        glDisable(GL_BLEND);
    }
}

// TODO: this is bad. Awful.
void Material::Unbind() {
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDepthFunc(GL_ALWAYS);

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
