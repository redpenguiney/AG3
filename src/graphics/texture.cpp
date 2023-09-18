#pragma once
#include "../../external_headers/GLEW/glew.h"
#include "../../external_headers/GLM/ext.hpp"
#include "../../external_headers/freetype/ft2build.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include "../../external_headers/stb/stb_image.h" 
#include "../debug/debug.cpp"

// TODO: world_fragment.glsl expects a texture array, we need a uniform to let it also sample from a normal texture 

enum TextureType {
    TEXTURE_2D = GL_TEXTURE_2D,
    TEXTURE_2D_ARRAY = GL_TEXTURE_2D_ARRAY,
    FONT = 0
};

class Texture {
    public:
    // read image file to texture. 
    // if type == TEXTURE_2D_ARRAY, the file will be treated as an array of height/layerHeight images that each have a height of layerHeight. That means if you want to create a texture array from a single file, the images must be in a column, not a row. 
    // if layerHeight == -1, layerHeight will be set to height. 
    // if type == FONT, not much will happen because i haven't added that yet.
    Texture(TextureType textureType, std::string path, int layerHeight = -1, int mipmapLevels = 4) {
        // use stbi_image.h to load file
        type = textureType;
        bindingLocation = (textureType == FONT) ? GL_TEXTURE_2D : textureType;
        unsigned char* imageData = stbi_load(path.c_str(), &width, &height, &nChannels, 4);
        if (stbi_failure_reason()) { // error check
            std::printf("STBI failed to load %s because %s", path.c_str(), stbi_failure_reason());
        }

        // generate OpenGL texture object and put image data in it
        glGenTextures(1, &glTextureId);
        Use();
        if (type == TEXTURE_2D) {
            depth = 1;

            glTexImage2D(bindingLocation, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
        }
        else if (type == TEXTURE_2D_ARRAY) {
            if (layerHeight == -1) {layerHeight = height;}
            depth = height/layerHeight;
            height = layerHeight;
            glTexImage3D(bindingLocation, 0, GL_RGBA8, width, height, depth, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
        }

        stbi_image_free(imageData);

        ConfigTexture();
    }

    // 16 x 64 texture:
        // width = 16
        // height = 64

    // read many image files to texture array, where each image becomes one layer.
    // Every image file must have the same size due to OpenGL requirements.
    // texture type must be TEXTURE_2D_ARRAY, or a 3d texture if i ever have support for that 
    Texture(TextureType textureType, std::vector<std::string>& paths, int mipmapLevels = 4) {
        type = textureType;
        bindingLocation = textureType;
        assert(type == TEXTURE_2D_ARRAY);

        // load the requested files
        std::vector<unsigned char*> imageDatas;
        
        int x = -1;
        int y = -1;
        for (auto & path: paths) {
            // use stbi_image.h to load file
            int imageX, imageY;
            unsigned char* imageData = stbi_load(path.c_str(), &imageX, &imageY, &nChannels, 4); 
            imageDatas.push_back(imageData);
            // make sure images are the same size
            if ((x != -1 && imageX != x) || (y != -1 && imageY != y)) {
                std::printf("images are not the same size");
                abort();
            }
            else {
                x = imageX;
                y = imageY;
            }

            if (stbi_failure_reason()) { // error check
                std::printf("STBI failed to load %s because %s", path.c_str(), stbi_failure_reason());
            }
        }
        width = x;
        height = y;

        // generate OpenGL texture object and put image data in it
        glGenTextures(1, &glTextureId);
        Use();
        glTexImage3D(bindingLocation, mipmapLevels, GL_RGBA, width, height, imageDatas.size(), 0, GL_RGBA, GL_UNSIGNED_INT, nullptr);
        unsigned int i = 0;
        for (auto data: imageDatas) {
            glTexSubImage3D(bindingLocation, 0, 0, 0, i, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
            i++;
        }

        ConfigTexture();
    }

    ~Texture() {
        glDeleteTextures(1, &glTextureId);
    }

    glm::uvec3 GetSize() {
        return glm::uvec3(width, height, depth);
    }

    void Use() {
        glBindTexture(bindingLocation, glTextureId);
    }

    private:
    GLuint glTextureId;
    GLenum bindingLocation;
    int width, height, depth, nChannels; // if the texture is not an array texture or a 3d texture, depth = 1
    TextureType type;

    // Sets all of the OpenGL texture parameters.
    void ConfigTexture() {
        Use();
        glTexParameteri(bindingLocation, GL_TEXTURE_WRAP_S, GL_REPEAT); // GL_REPEAT means tile the texture
        glTexParameteri(bindingLocation, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(bindingLocation, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(bindingLocation, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
};