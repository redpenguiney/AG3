#pragma once
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>
#include "texture.hpp"
#include <optional>

// A material is just a collection of textures.
// On the GPU, each object has a materialId that it uses to get color, normal, etc. textures.
// On the CPU, meshpools are sorted by their material to minimize texture bindings.
class Material {
    public:
    enum MaterialTextureIndex {
        COLORMAP_TEXTURE_INDEX = 0,
        NORMALMAP_TEXTURE_INDEX = 1,
        SPECULARMAP_TEXTURE_INDEX = 2,
        DISPLACEMENTMAP_TEXTURE_INDEX = 3,
        FONTMAP_TEXTURE_INDEX = 4
    };

    const unsigned int id;
    const Texture::TextureType materialType;

    bool HasColorMap();
    bool HasSpecularMap();
    bool HasNormalMap();
    bool HasDisplacementMap();
    bool HasFontMap();

    // if false, things drawn with this material won't write to the depth buffer. Should be true unless you're doing something weird with transparency, like fonts.
    const bool depthMaskEnabled;

    // Returns a ptr to the material with the given id.
    static std::shared_ptr<Material>& Get(const unsigned int id);

    // TODO: figure out what happens when you call this while the texture is being used.
    static void Destroy(const unsigned int id);

    // makes all things be drawn with these textures (until Use()/Unbind() is called on another material)
    void Use();

    // makes all things not be drawn with any material (until Use() is called on a material)
    static void Unbind();

    // textureParams must at minimum contain a TextureCreateParams for color.
    // If possible, will not to create a new material, but simply add the requested textures to an existing compatible material.
    // Returns a pair of (textureZ, ptr to the created material).
    // Take care that any shaders you use with this material actually use the requested color/normal/specular.
    // TODO: default normal/specular option?
    static std::pair<float, std::shared_ptr<Material>> New(const std::vector<TextureCreateParams>& textureParams, Texture::TextureType type, const bool depthMask = true);

    //~Material(); implicit destructor fine
    
    // read only access to materials (TODO other ones if needed)
    const std::optional<Texture>& colorMapConstAccess = colorMap;
    const std::optional<Texture>& fontMapConstAccess = fontMap;

    private:
    
    // For optimization purposes, when you ask for a new texture, it will try to simply add a new layer to an existing material of the same size if it can.
    // If the given textures are compatible with this material and it successfully adds a new layer for them, it will return the textureZ they were put on.
    // If not, it returns std::nullopt.
    std::optional<float> TryAppendLayer(const std::vector<TextureCreateParams>& textureParams, Texture::TextureType type);

    Material(const std::vector<TextureCreateParams>& textureParams, Texture::TextureType type, const bool depthMask);

    std::optional<Texture> colorMap;
    std::optional<Texture> normalMap; 
    std::optional<Texture> specularMap;
    std::optional<Texture> displacementMap;
    std::optional<Texture> fontMap;

};