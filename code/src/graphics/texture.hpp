#pragma once
#include "GL/glew.h"
#include <glm/vec3.hpp>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <variant>

// TODO: we need to get textures to actually tile
// TODO: BINDLESS TEXTURE SUPPORT would be awesome
// TODO: enum class instead of enum

struct TextureCreateParams;

// Contains information about a specific characte of a specific font, and how to draw it from a font texture.
struct Glyph {
    unsigned int width;
    unsigned int height;

    unsigned int advance; // how many pixels the next glyph should start after this one

    int bearingX; // offset from baseline to left of glyph
    int bearingY; // offset from baseline to bottom of glyph

    GLfloat leftUv; // X texture coordinate for left side of glyph on the rasterized font's texture atlas
    GLfloat rightUv; // X texture coordinate for right side of glyph on the rasterized font's texture atlas
    GLfloat topUv; // Y texture coordinate for top side of glyph on the rasterized font's texture atlas
    GLfloat bottomUv; // Y texture coordinate for bottom side of glyph on the rasterized font's texture atlas

};

class Framebuffer;

class aiScene; // assimp can load models that have embedded textures. In those cases, the texture constructor needs to ask the assimp scene for the embedded texture.

class Texture {
    public:

    // whether texture is 2d, 3d, cubemap, etc.
    // TODO: why caps lock
    enum TextureType {
        
        Texture2D = 0,
        TextureCubemap = 1, // cubemap is for skybox
    };

    enum TextureFormat {
        RGBA_8Bit = GL_RGBA8,
        RGB_8Bit = GL_RGB8,
        Grayscale_8Bit = GL_R8,
        Auto_8Bit = 0, // texture format will be identical to the file's format (but still rgb)

        RGBA_16Float = GL_RGBA16F // you don't need 16 bit textures for objects, these are for things like floating point framebuffers for HDR
    };

    enum TextureWrappingBehaviour {
        WrapTiled = 0, // texture is tiled (UV of (1.5, 1.5) == UV of (0.5, 0.5))
        WrapMirroredTiled = 1, // texture is mirrored and tiled (U/V of -0.2 becomes 0.2)
        WrapMirroredClamped = 2, // texture is mirrored and clamped to edge after first mirror
        WrapClampToEdge = 3, // UVs are clamped to edge (UV of (1.5, 1.5) == UV of (1, 1))
    };

    // TODO: antistrophic filtering
    enum TextureFilteringBehaviour {
        NoTextureFiltering = 0, // pixels of texture remain sharp and crisp (like Minecraft)
        LinearTextureFiltering = 1, // interpolation between pixels of texture (basically blurs textures when looking up close to hide the pixels a little)
    };

    enum TextureMipmapBehaviour {
        NoMipmaps = 0, // do not use mipmapping
        UseNearestMipmap = 1, // use closest mipmap for the texture based on distacne
        LinearMipmapInterpolation = 2, // smoothly interpolate between mipmaps with distance, may reduce moire patterns/artifacts
    };

    enum TextureUsage {
        ColorMap = 0,
        SpecularMap = 1,
        NormalMap = 2,
        DisplacementMap = 3,
        FontMap = 4
    };

    // lets textureId be read only without a getter
    const unsigned int& textureId = glTextureId;

    const TextureFormat format;

    // normal constructor for a texture. TextureIndex refers to which texture unit it will occupy when bound (when rendering, each texture unit can hold one texture for shaders to access)
    // Aborts if arguments are inherently invalid, but throws an exception if texture files is nonexistent/incompatible with those arguments.
    
    Texture(const TextureCreateParams& params, const GLuint textureIndex, const TextureType textureType); 

    // constructor that creates an empty texture (params.texturePaths is ignored and should be an empty vector) and binds it to the given framebuffer.
    Texture(Framebuffer& framebuffer, const TextureCreateParams& params, const GLuint textureIndex, const TextureType textureType, const GLenum framebufferAttachmentType); 
    
    Texture(const Texture&) = delete; // destructor deletes the openGL texture, so copying it would be bad since it would be using the same id of the now-deleted openGL texture 

    Texture(Texture&&) = default; // move constructor is allowed

    // Given that the texture is an array texture, will append the image at the given path to the texture array, returning the textureZ coordinate the image can be accessed through.
    float AddLayer();

    
    // static std::shared_ptr<Texture> New(TextureType textureType, std::string path, int layerHeight = -1, int mipmapLevels = 4);

    // static std::shared_ptr<Texture> New(TextureType textureType, std::vector<std::string>& paths, int mipmapLevels = 4);

    ~Texture();

    glm::uvec3 GetSize();

    // Makes OpenGL draw everything with this texture, until Use() is called on a different texture.
    void Use();

    const TextureType type;
    const TextureUsage usage;

    // read-only access to fontGlyphs
    const std::optional<std::unordered_map<char, Glyph>>& glyphs = fontGlyphs; 

    // distance between each line, if it is a font texture
    const GLfloat lineSpacing;

    private:
    int width, height, depth, nChannels; // if the texture is not an array texture or a 3d texture, depth = 1

    GLuint glTextureId; // id is used by opengl
    const GLenum bindingLocation; // basically what kind of opengl texture it is; cubemap, 3d, 2d, 2d array, etc.
    const GLuint glTextureIndex; // multiple textures can be bound at once; this is which index it is bound to
    
    

    // Sets all of the OpenGL texture parameters.
    void ConfigTexture(const TextureCreateParams& params);

    // Texture(TextureType textureType, std::string path, int layerHeight, int mipmapLevels);
    // Texture(TextureType textureType, std::vector<std::string>& paths, int mipmapLevels);

    // If this texture is a font, then this map exists and will store information (uvs, spacing, etc.) for each character
    std::optional<std::unordered_map<char, Glyph>> fontGlyphs;
};

struct Image {
    // array of pixels. Image does NOT own.
    uint8_t* imageData;

    // based on length of imageData
    int width, height, nChannels;

    // format of imageData
    Texture::TextureFormat format;
};

// Generates (or already contains) an image.
// Basically MeshProvider but for textures and simpler.
class TextureSource {
public:
    TextureSource(std::string imagePath);

    // Texture does NOT take ownership of the imageData pointer of the given image. Deleting that pointer is still your problem.
    TextureSource(Image image);

    // returns the contained Image if it was provided, or otherwise loads the filepath to generate an Image and returns that.
    Image GetImage();

private:
    const std::variant<std::string, Image> imageData;
};

struct TextureCreateParams {
    // Unless creating cubemap, size must = 1.
    // If it is a cubemap, it goes Right,Left,Top,Bottom,Back,Front
    const std::vector<TextureSource> texturePaths; 

    // how texture data is stored on the GPU; RGB, RGBA, grayscale for things like heightmaps, etc.
    // NOT how it is stored within the file, we automatically determine that.
    Texture::TextureFormat format;

    // whether texture is for color, normals, specular, etc.
    const Texture::TextureUsage usage;

    // what happens when a shader tries to sample outside the texture's area, whether the texture tiles or does something else
    Texture::TextureWrappingBehaviour wrappingBehaviour;

    // when a pixel on the screen occupies multiple pixels on a texture, this is how it comes up with a final texture to use.
    Texture::TextureFilteringBehaviour filteringBehaviour;

    // whether a texture uses mipmaps and if so, how it interpolates between them (if at all)
    Texture::TextureMipmapBehaviour mipmapBehaviour;

    TextureCreateParams(const std::vector<std::string>& imagePaths, const Texture::TextureUsage usage);

    // TextureCreateParams does NOT own the aiScene*. Pass it if the texture path came from assimp (for support for textures embedded in a fbx/dae/etc.)
    // Used for textures embedded in a model file like .fbx 
    const aiScene* scene = nullptr;

    unsigned int fontHeight; // font width is automatically calculated
};