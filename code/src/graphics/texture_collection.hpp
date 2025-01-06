#pragma once
#include <utility>
#include <optional>
#include <array>
#include "texture.hpp"

class MaterialCreateParams;

// Materials can share their textures (by storing them in texture arrays, for example), so materials do not directly store their textures but instead hold references to TextureCollections.
class TextureCollection {
public:
    const unsigned int id;

    // in OpenGL 3.0+ we're guaranteed to be able to bind up to 16 textures at a time (16 texture units)
    std::array<std::optional<Texture>, 16> textures;

    const Texture::TextureType textureType;

    const bool autoAppendationAllowed;

    // Creates or appends or whatevers 
    static std::pair<std::shared_ptr<TextureCollection>, float> FindCollection(const MaterialCreateParams& params);

    // only public for std::make_shared (TODO mid code lol)
    TextureCollection(const MaterialCreateParams& params);

private:
    static inline unsigned int lastId = 0;
};