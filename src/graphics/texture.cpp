#pragma once
#include "engine.hpp"
#include "../../external_headers/GLEW/glew.h"
#include "../../external_headers/GLM/ext.hpp"
#include "../../external_headers/freetype/ft2build.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include "../../external_headers/stb/stb_image.h" 
#include "../debug/debug.cpp"
#include "texture.hpp"

// read image file to texture. 
// if type == TEXTURE_2D_ARRAY, the file will be treated as an array of height/layerHeight images that each have a height of layerHeight. That means if you want to create a texture array from a single file, the images must be in a column, not a row. 
// if type == TEXTURE_2D_ARRAY, the image may be appended to 
// if layerHeight == -1, layerHeight will be set to height, meaning the whole image will be loaded into a single layer. 
// if type == FONT, not much will happen because i haven't added that yet.
std::shared_ptr<Texture> Texture::Texture::New(TextureType textureType, std::string path, int layerHeight, int mipmapLevels) {
    auto ptr = std::shared_ptr<Texture>(new Texture(textureType, path, layerHeight, mipmapLevels));
    LOADED_TEXTURES.emplace(ptr->glTextureId, ptr);
    return ptr;
}

// read many image files to texture array, where each image becomes one layer.
// Every image file must have the same size due to OpenGL requirements.
// texture type must be TEXTURE_2D_ARRAY, TEXTURE_CUBEMAP, or a 3d texture if i ever have support for that.
// if texture type == TEXTURE_CUBEMAP, paths must be in the order {right, left, top, bottom, back, front}
std::shared_ptr<Texture> Texture::New(TextureType textureType, std::vector<std::string>& paths, int mipmapLevels) {
    auto ptr = std::shared_ptr<Texture>(new Texture(textureType, paths, mipmapLevels));
    LOADED_TEXTURES.emplace(ptr->glTextureId, ptr);
    return ptr;
}

// Get a pointer to a texture by its id.
std::shared_ptr<Texture>& Texture::Get(unsigned int textureId) {
    assert(LOADED_TEXTURES.count(textureId) != 0 && "Texture::Get() was given an invalid textureId.");
    return LOADED_TEXTURES[textureId];
}

// unloads the texture with the given textureId, freeing its memory.
// Calling this function while objects still use the texture will error.
// You only need to call this if there are more textures than can fit in VRAM.
void Texture::Unload(unsigned int textureId) {
    assert(GraphicsEngine::Get().IsTextureInUse(textureId));
    assert(LOADED_TEXTURES.count(textureId) != 0 && "Texture::Unload() was given an invalid textureId.");
    LOADED_TEXTURES.erase(textureId);
}

Texture::~Texture() {
    glDeleteTextures(1, &glTextureId);
}

glm::uvec3 Texture::GetSize() {
    return glm::uvec3(width, height, depth);
}

// Makes OpenGL draw everything with this texture, until Use() is called on a different texture.
void Texture::Use() {
    glBindTexture(bindingLocation, glTextureId);
}

// Sets all of the OpenGL texture parameters.
void Texture::ConfigTexture() {
    Use();
    glTexParameteri(bindingLocation, GL_TEXTURE_WRAP_S, GL_REPEAT); // GL_REPEAT means tile the texture
    glTexParameteri(bindingLocation, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(bindingLocation, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(bindingLocation, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(bindingLocation);
}

Texture::Texture(TextureType textureType, std::string path, int layerHeight, int mipmapLevels) {
    // use stbi_image.h to load file
    type = textureType;
    bindingLocation = (textureType == TEXTURE_FONT) ? GL_TEXTURE_2D : textureType;
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

Texture::Texture(TextureType textureType, std::vector<std::string>& paths, int mipmapLevels) {
    type = textureType;
    bindingLocation = textureType;
    assert(type == TEXTURE_2D_ARRAY || type == TEXTURE_CUBEMAP);

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
    if (type == TEXTURE_CUBEMAP) {
        depth = 1;
    }
    else {
        depth = imageDatas.size();
    }
    

    // generate OpenGL texture object and put image data in it
    glGenTextures(1, &glTextureId);
    Use();

    if (type != TEXTURE_CUBEMAP) {
        glTexImage3D(bindingLocation, mipmapLevels, GL_RGBA, width, height, imageDatas.size(), 0, GL_RGBA, GL_UNSIGNED_INT, nullptr);
    }

    unsigned int i = 0;
    for (auto data: imageDatas) {
        if (type == TEXTURE_2D_ARRAY) {
            glTexSubImage3D(bindingLocation, 0, 0, 0, i, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        else if (type == TEXTURE_CUBEMAP) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        stbi_image_free(data);
        i++;
    }

    ConfigTexture();
}