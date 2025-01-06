#include "texture_collection.hpp"
#include "material.hpp"
#include <debug/assert.hpp>

TextureCollection::TextureCollection(const MaterialCreateParams& params) :
	autoAppendationAllowed(params.allowAppendaton),
	id(lastId++),
    textureType(params.type)
{
	Assert(params.srcTextures == nullptr);
	
    // Just go through textureParams and create each texture they ask for
    // TODO: lots of sanity checks should be made here on the params for each texture
    unsigned int i = 0;
    for (auto& textureToMake : params.textureParams) {

        //Assert(Count(textureToMake.usage) == 0);

        switch (textureToMake.usage) {
        case Texture::ColorMap:
            textures.at(i).emplace(textureToMake, Material::COLORMAP_TEXTURE_INDEX, params.type); // constructs the texture inside the optional without copying
            break;
        case Texture::NormalMap:
            textures.at(i).emplace(textureToMake, Material::NORMALMAP_TEXTURE_INDEX, params.type); // constructs the texture inside the optional without copying
            break;
        case Texture::SpecularMap:
            textures.at(i).emplace(textureToMake, Material::SPECULARMAP_TEXTURE_INDEX, params.type); // constructs the texture inside the optional without copying
            break;
        case Texture::DisplacementMap:
            textures.at(i).emplace(textureToMake, Material::DISPLACEMENTMAP_TEXTURE_INDEX, params.type); // constructs the texture inside the optional without copying
            break;
        case Texture::FontMap:
            textures.at(i).emplace(textureToMake, Material::FONTMAP_TEXTURE_INDEX, params.type); // constructs the texture inside the optional without copying
            break;
        default:
            DebugLogError("texture colection constructor: what are you doing???\n");
            abort();
            break;
    }

    i++;
}
}

// TODO: when you actually add appendation here, have a system to track which textureZs are owned by which materials so they can be freed up as needed.
std::pair<std::shared_ptr<TextureCollection>, float> TextureCollection::FindCollection(const MaterialCreateParams& params) {
    return std::make_pair(std::make_shared<TextureCollection>(params), 0); // TODO: finding preexisting texture collection instead if valid
}