#pragma once
#include "../../external_headers/GLEW/glew.h"
#include "../../freetype/ft2build.h"
#include <string>

enum TextureType {
    TEXTURE_2D = GL_TEXTURE_2D,
    TEXTURE_2D_ARRAY = GL_TEXTURE_2D_ARRAY
};

class Texture {
    public:
    // read image to texture
    Texture(TextureType type, std::string path) {

    }

    // ttf font + text to texture
    Texture(std::string fontPath, std::string text) {

    }

    ~Texture() {
        glDeleteTextures(1, &glTextureId);
    }

    private:
    GLuint glTextureId;
};