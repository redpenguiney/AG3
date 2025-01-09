#pragma once
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>
#include <array>
#include "texture.hpp"
#include <optional>
#include "shader_input_provider.hpp"
#include "texture_collection.hpp"
#include "glm/vec2.hpp"

class ShaderProgram;

enum class DepthTestMode: GLenum {
    Disabled = GL_ALWAYS, // values in depth buffer are ignored for rendering purposes.
    Less = GL_LESS,
    LEqual = GL_LEQUAL // Use this for normal stuff.
};

// See https://learnopengl.com/Advanced-OpenGL/Blending
enum class BlendFactorMode: GLenum {
    Zero = GL_ZERO,
    One = GL_ONE,
    SrcColor = GL_SRC_COLOR,
    SrcAlpha = GL_SRC_ALPHA,
    OneMinusSrcColor = GL_ONE_MINUS_SRC_COLOR,
    OneMinusSrcAlpha = GL_ONE_MINUS_SRC_ALPHA,
    DstColor = GL_DST_COLOR,
    DstAlpha = GL_DST_ALPHA,
    OneMinusDstColor = GL_ONE_MINUS_DST_COLOR,
    OneMinusDstAlpha = GL_ONE_MINUS_DST_ALPHA,
};

struct MaterialCreateParams {


    // textureParams can be empty; if so there will be no textures associated with the material.
    // Do not use if you supply srcTextures.
    std::vector<TextureCreateParams> textureParams;

    Texture::TextureType type;

    // The shader program objects drawn with this material will use. If nullptr, the default shader program supplied by GraphicsEngine will be used.
    std::shared_ptr<ShaderProgram> shader = nullptr;

    // If you supply this INSTEAD of textureParams, the material will just use these textures.
    std::shared_ptr<TextureCollection> srcTextures = nullptr;

    // set to false for fonts. 
    // Determines whether objects drawn with this material will affect the depth buffer (which will affect whether objects behind them could be drawn over them).
    // For pretty much everything this should be set to true.
    bool depthMask = true;

    // if true, the material's texture collection will not INITIALLY be shared with other materials by appending its own textures to the collection (which is done as a drawcall optimization).
    // Other materials may still be appended to it unless allowAppendation is also false.
    bool requireUniqueTextureCollection = false;

    // If false, other materials will not be able to append their textures to this one's texture collection (as a drawcall optimization) unless you explicitly do it yourself.
    bool allowAppendaton = true;

    ShaderInputProvider inputProvider = ShaderInputProvider();

    
    // Determines whether (and if so, how) fragments (pixels) of a rendered object will read from the depth buffer to decide whether they are obscured.
    // Doesn't affet whether rendering will WRITE to the depth buffer, however; that's what depthMaskEnabled is about.
    DepthTestMode depthTestFunc = DepthTestMode::LEqual;

    bool blendingEnabled = true;
    // must have equal amount of each factor
    std::vector<BlendFactorMode> blendingSrcFactor = { BlendFactorMode::SrcAlpha };
    std::vector<BlendFactorMode> blendingDstFactor = { BlendFactorMode::OneMinusSrcAlpha };

    int drawOrder = 0;
};

// A material is a collection of textures and a collection of uniforms/other shader program inputs.
// On the GPU, each object has a materialId that it uses to get color, normal, etc. textures.
// On the CPU, meshpools are sorted by their material to minimize texture bindings.
// Note: pretty much all members should be const for appendation support
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

    // The shader program objects drawn with this material will use.
    std::shared_ptr<ShaderProgram> shader;

    // set to false for fonts. 
    // Determines whether objects drawn with this material will affect the depth buffer (which will affect whether objects behind them could be drawn over them).
    // For pretty much everything this should be set to true.
    bool depthMaskEnabled;

    bool ignorePostProc = false; // if true, stuff with this shader will be rendered after postprocessing (on top of the screen quad)

    // Determines whether (and if so, how) fragments (pixels) of a rendered object will read from the depth buffer to decide whether they are obscured.
    // Doesn't affet whether rendering will WRITE to the depth buffer, however; that's what depthMaskEnabled is about.
    DepthTestMode depthTestFunc;

    // Scissor test
    bool scissoringEnabled = false;
    glm::ivec2 scissorCorner1;
    glm::ivec2 scissorCorner2;

    bool blendingEnabled;
    // blending factors for each render target if multiple
    std::vector<BlendFactorMode> blendingSrcFactor;
    std::vector<BlendFactorMode> blendingDstFactor;

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
    //static std::pair<float, std::shared_ptr<Material>> NewCopy(const std::shared_ptr<Material>& original);

    //~Material(); implicit destructor fine
    
    // returns how many textures in the texture collection are for the specific usage (like color)
    unsigned int Count(Texture::TextureUsage texUsage) const;

    // requires that Count(type) == 1
    const Texture& Get(Texture::TextureUsage texUsage) const;

    // may be nullptr if the material has no textures.
    std::shared_ptr<TextureCollection> textures;

    // textureZ/depth value of the first texture in the collection that this material specifically cares about.
    float baseTextureZ = 0.0;

    ShaderInputProvider inputProvider;

    // Objects with different draw order values will be drawn in order of least to greatest. Do not change unless neccesary; it prevents the optimizing away of redundant GL state changes.
    int drawOrder;
    
private:
    
    
    
    // For optimization purposes, when you ask for a new texture, it will try to simply add a new layer to an existing material of the same size if it can.
    // If the given textures are compatible with this material and it successfully adds a new layer for them, it will return the textureZ they were put on.
    // If not, it returns std::nullopt.
    //std::optional<float> TryAppendLayer(const MaterialCreateParams& params);

    // private constructor to enforce usage of factory method
    Material(const MaterialCreateParams& params);

    friend class Meshpool;

};