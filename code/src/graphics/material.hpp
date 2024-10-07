#pragma once
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>
#include <array>
#include "texture.hpp"
#include <optional>
#include "shader_input_provider.hpp"

struct MaterialCreateParams {

    // textureParams must at minimum contain a TextureCreateParams for color.
    std::vector<TextureCreateParams> textureParams;


    Texture::TextureType type;

    // set to false for fonts. 
    // Determines whether objects drawn with this material will affect the depth buffer (which will affect whether objects behind them could be drawn over them).
    // For pretty much everything this should be set to true.
    bool depthMask = true;

    // if true, the material will not be appended to any other materials (as a drawcall optimization).
    // Other materials may still be appended to it unless allowAppendation is also false.
    bool requireSingular = false;

    // If false, other materials will not be able to be appended to this one (as a drawcall optimization) unless you explicitly do it.
    bool allowAppendaton = true;

    ShaderInputProvider inputProvider = ShaderInputProvider();
};

// A material is a collection of textures and a collection of uniforms/other shader program inputs.
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

    //bool HasColorMap();
    //bool HasSpecularMap();
    //bool HasNormalMap();
    //bool HasDisplacementMap();
    //bool HasFontMap();

    // if false, things drawn with this material won't write to the depth buffer. Should be true unless you're doing something weird with transparency, like fonts.
    const bool depthMaskEnabled;

    // Returns a ptr to the material with the given id.
    static std::shared_ptr<Material>& Get(const unsigned int id);

    // TODO: figure out what happens when you call this while the texture is being used.
    static void Destroy(const unsigned int id);

    // makes all things be drawn with these textures (until Use()/Unbind() is called on another material)
    // Needs shader so it can pass uniforms/etc. to it via its inputProvider.
    void Use(std::shared_ptr<ShaderProgram> currentShader);

    // makes all things not be drawn with any material (until Use() is called on a material)
    static void Unbind();

    
    // If possible, will not to create a new material, but simply add the requested textures to an existing compatible material.
    // Returns a pair of (textureZ, ptr to the created material).
    // Take care that any shaders you use with this material actually use the requested color/normal/specular.
    // TODO: default normal/specular option?
    // Aborts if arguments are inherently invalid, but throws an exception if texture files is nonexistent/incompatible with those arguments.
    static std::pair<float, std::shared_ptr<Material>> New(const MaterialCreateParams& params);

    // Creates a new material that shares the same textures as the original but can have different uniform/shader inputs.
    static std::pair<float, std::shared_ptr<Material>> NewCopy(const std::shared_ptr<Material>& original);

    //~Material(); implicit destructor fine
    
    struct TextureCollection {
        // in OpenGL 3.0+ we're guaranteed to be able to bind up to 16 textures at a time (16 texture units)
        std::array<std::optional<Texture>, 16> textures;

        // returns how many textures in the collection are for the specific usage (like color)
        unsigned int Count(Texture::TextureUsage texUsage);
    };

    // read only access to textures
    const std::shared_ptr<TextureCollection>& GetTextureCollection() const;

    const ShaderInputProvider inputProvider;
    
private:
    
    // never nullptr
    std::shared_ptr<TextureCollection> textures;
    
    // For optimization purposes, when you ask for a new texture, it will try to simply add a new layer to an existing material of the same size if it can.
    // If the given textures are compatible with this material and it successfully adds a new layer for them, it will return the textureZ they were put on.
    // If not, it returns std::nullopt.
    std::optional<float> TryAppendLayer(const MaterialCreateParams& params);

    // private constructor to enforce usage of factory method
    Material(const MaterialCreateParams& params);

    

};