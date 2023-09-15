#pragma once
#include "../../external_headers/GLEW/glew.h"
#include "../../external_headers/GLM/ext.hpp"
#include "../../freetype/ft2build.h"
#include <cassert>
#include <string>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include "../../external_headers/stb/stb_image.h" 

enum TextureType {
    TEXTURE_2D = GL_TEXTURE_2D,
    TEXTURE_2D_ARRAY = GL_TEXTURE_2D_ARRAY,
    FONT = 0
};

class Texture {
    public:
    // read image file to texture. 
    // if type == TEXTURE_2D_ARRAY and layerWidth > 0, the file will be treated as an array of images that each have a height of layerHeight. That means if you want to create a texture array from a single file, the images must be in a column, not a row.
    Texture(TextureType type, std::string path, int layerHeight = -1) {
        assert(type != FONT);

        // use stbi_image.h to load file
        unsigned char* imageData = stbi_load(path.c_str(), &width, &height, &nChannels, 4); 

        // generate OpenGL texture object and put image data in it
        glGenTextures(1, &glTextureId);
        glBindTexture(type, glTextureId);
        if (type == TEXTURE_2D) {
            glTexImage2D(GL_TEXTURE_2D)
        }
    }

    // read many image files to texture array, where each image becomes one layer.
    // Every image file must have the same size due to OpenGL requirements.
    // texture type must be TEXTURE_2D_ARRAY, or a 3d texture if i ever have support for that 
    Texture(TextureType type, std::vector<std::string>& paths) {
        assert(type == TEXTURE_2D_ARRAY);
    }

    // ttf font + text to texture
    Texture(std::string fontPath, std::string text) {
        
    }

    ~Texture() {
        glDeleteTextures(1, &glTextureId);
    }

    glm::uvec2 GetSize() {
        return glm::uvec2(width, height);
    }

    private:
    GLuint glTextureId;
    int width, height, nChannels;
};