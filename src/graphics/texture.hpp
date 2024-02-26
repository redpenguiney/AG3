#pragma once
#include "../../external_headers/GLEW/glew.h"
#include "../../external_headers/GLM/vec3.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// TODO: we need to get textures to actually tile
// TODO: BINDLESS TEXTURE SUPPORT would be awesome

struct TextureCreateParams;

class Texture {
    public:

    // whether texture is 2d, 3d, cubemap, etc.
    // TODO: why caps lock
    enum TextureType {
        Texture2D = 0,
        TextureCubemap = 1, // cubemap is for skybox
        TextureFont = 2
    };

    enum TextureFormat {
        RGBA = GL_RGBA8,
        RGB = GL_RGB8,
        Grayscale = GL_R8,
        Auto = 0, // texture format will be identical to the file's format
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
        DisplacementMap = 3
    };

    // lets textureId be read only without a getter
    const unsigned int& textureId = glTextureId;

    const TextureFormat format;

    Texture(const TextureCreateParams& params, const GLuint textureIndex, TextureType textureType);
    Texture(const Texture&) = delete; // destructor deletes the openGL texture, so copying it would be bad

    // Given that the texture is an array texture, will append the image at the given path to the texture array, returning the textureZ coordinate the image can be accessed through.
    float AddLayer();

    
    // static std::shared_ptr<Texture> New(TextureType textureType, std::string path, int layerHeight = -1, int mipmapLevels = 4);

    // static std::shared_ptr<Texture> New(TextureType textureType, std::vector<std::string>& paths, int mipmapLevels = 4);

    ~Texture();

    glm::uvec3 GetSize();

    // Makes OpenGL draw everything with this texture, until Use() is called on a different texture.
    void Use();

    private:
    int width, height, depth, nChannels; // if the texture is not an array texture or a 3d texture, depth = 1

    GLuint glTextureId; // id is used by opengl
    const GLenum bindingLocation; // basically what kind of opengl texture it is; cubemap, 3d, 2d, 2d array, etc.
    const GLuint glTextureIndex; // multiple textures can be bound at once; this is which index it is bound to
    
    const TextureType type;

    // Sets all of the OpenGL texture parameters.
    void ConfigTexture(const TextureCreateParams& params);

    Texture(TextureType textureType, std::string path, int layerHeight, int mipmapLevels);
    Texture(TextureType textureType, std::vector<std::string>& paths, int mipmapLevels);
};

struct TextureCreateParams {
    const std::vector<std::string> texturePaths; // Note: unless creating cubemap, size must = 1.

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
};