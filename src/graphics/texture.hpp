#pragma once
#include "../../external_headers/GLEW/glew.h"
#include "../../external_headers/GLM/ext.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// TODO: BINDLESS TEXTURE SUPPORT would be awesome

// whether texture is 2d, 3d, cubemap, etc.
enum TextureType {
    TEXTURE_2D = 0,
    TEXTURE_CUBEMAP = 1, // cubemap is for skybox
    TEXTURE_FONT = 2
};

// how texture data is stored on the GPU; RGB, RGBA, grayscale for things like heightmaps, etc.
// NOT how it is stored within the file.
enum TextureFormat {
    RGBA = GL_RGBA8,
    RGB = GL_RGB8,
    Grayscale = GL_R8
};

// whether texture is for color, normals, specular, etc.
enum TextureUsage {
    Color = 0,
    Specular = 1,
    Normal = 2,
    Height = 3
};

struct TextureCreateParams {
    std::vector<std::string> texturePaths; // Note: unless creating cubemap, size must = 1.
    TextureType textureType;
    TextureFormat format;
    TextureUsage usage;
};

class Texture {
    public:
    // lets textureId be read only without a getter
    const unsigned int& textureId = glTextureId;

    const TextureFormat format;

    Texture(const TextureCreateParams& params, const GLuint textureIndex);
    Texture(const Texture&) = delete;

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
    void ConfigTexture();

    Texture(TextureType textureType, std::string path, int layerHeight, int mipmapLevels);
    Texture(TextureType textureType, std::vector<std::string>& paths, int mipmapLevels);
};