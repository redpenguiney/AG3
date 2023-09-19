#pragma once
#include "../../external_headers/GLEW/glew.h"
#include "../../external_headers/GLM/ext.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// TODO: world_fragment.glsl expects a texture array, we need a uniform to let it also sample from a normal texture 
enum TextureType {
    TEXTURE_2D = GL_TEXTURE_2D,
    TEXTURE_2D_ARRAY = GL_TEXTURE_2D_ARRAY,
    FONT = 0
};

class Texture {
    public:

    static unsigned int New(TextureType textureType, std::string path, int layerHeight = -1, int mipmapLevels = 4);
    static unsigned int New(TextureType textureType, std::vector<std::string>& paths, int mipmapLevels = 4);

    static std::shared_ptr<Texture>& Get(unsigned int textureId);

    static void Unload(unsigned int textureId);

    ~Texture();

    glm::uvec3 GetSize();

    void Use();

    private:
    inline static std::unordered_map<unsigned int, std::shared_ptr<Texture>> LOADED_TEXTURES; 

    GLuint textureId; // id is used both by opengl and us as a uuid
    GLenum bindingLocation;
    int width, height, depth, nChannels; // if the texture is not an array texture or a 3d texture, depth = 1
    TextureType type;

    // Sets all of the OpenGL texture parameters.
    void ConfigTexture();

    Texture(TextureType textureType, std::string path, int layerHeight, int mipmapLevels);
    Texture(TextureType textureType, std::vector<std::string>& paths, int mipmapLevels);
};