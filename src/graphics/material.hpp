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
        DISPLACEMENTMAP_TEXTURE_INDEX = 3
    };

    const unsigned int id;
    const TextureType materialType;

    bool HasSpecularMap();
    bool HasNormalMap();
    bool HasDisplacementMap();

    // Returns a ptr to the material with the given id.
    static std::shared_ptr<Material>& Get(const unsigned int id);

    // TODO: figure out what happens when you call this while the texture is being used.
    static void Destroy(const unsigned int id);

    // makes all things be drawn with these textures
    void Use();

    // textureParams must at minimum contain a TextureCreateParams for color.
    // If possible, will not to create a new material, but simply add the requested textures to an existing compatible material.
    // Returns a pair of (textureZ, ptr to the created material).
    // Take care that any shaders you use with this material use the requested color/normal/specular.
    // TODO: default normal/specular option?
    static std::pair<float, std::shared_ptr<Material>> New(const std::vector<TextureCreateParams>& textureParams, TextureType type);

    //~Material(); implicit destructor fine
    
    private:
    static inline unsigned int LAST_MATERIAL_ID = 1;
    Material(const std::vector<TextureCreateParams>& textureParams, TextureType type);

    std::optional<Texture> colorMap;
    std::optional<Texture> normalMap; 
    std::optional<Texture> specularMap;
    std::optional<Texture> displacementMap;

    inline static std::unordered_map<unsigned int,  std::shared_ptr<Material>> MATERIALS;
};