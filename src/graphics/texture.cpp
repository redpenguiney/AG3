#include "engine.hpp"
#include "../../external_headers/GLEW/glew.h"
#include "../../external_headers/GLM/ext.hpp"
#include "../../external_headers/freetype/ft2build.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <process.h>
#include <string>
#include <unordered_map>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include "../../external_headers/stb/stb_image.h" 
#include "../debug/debug.hpp"
#include "texture.hpp"

GLenum TextureBindingLocationFromType(TextureType type) {
    switch (type) {
    case TEXTURE_2D:
    return GL_TEXTURE_2D_ARRAY;    
    break;
    case TEXTURE_CUBEMAP:
    return GL_TEXTURE_CUBE_MAP;
    break;
    default:
    std::cout << "forgot something\n";
    abort();
    
    break;
    }
    return 0; // gotta return something to hide the warning
}

unsigned int NChannelsFromFormat(TextureFormat format ) {
    switch (format) {
    case RGBA:
    return 4;
    break;
    case RGB:
    return 3;
    break;
    case Grayscale:
    return 1;
    default:
    std::cout << "forgot something\n";
    abort();
    break;
    }
    return 0; // gotta return something to hide the warning
}

// Create texture.
Texture::Texture(const TextureCreateParams& params, const GLuint textureIndex):
format(params.format),
bindingLocation(TextureBindingLocationFromType(params.textureType)),
glTextureIndex(textureIndex),
type(params.textureType)
{
    // skyboxes need 6 textures, everything else obviously only needs 1
    if (params.textureType == TEXTURE_CUBEMAP) {
        assert(params.texturePaths.size() == 6);
    }
    else {
        assert(params.texturePaths.size() == 1);
    }

    std::vector<unsigned char*> imageDatas;
    
    unsigned int lastWidth = 0, lastHeight = 0;
    for (auto & path: params.texturePaths) {
        // use stbi_image.h to load file
        imageDatas.push_back(stbi_load(path.c_str(), &width, &height, &nChannels, NChannelsFromFormat(params.format)));
        if (stbi_failure_reason()) { // error check
            std::printf("STBI failed to load %s because %s", path.c_str(), stbi_failure_reason());
            abort(); // TODO: set image data to pointer to an error texture when this happens instead of aborti
        }
        if ((lastWidth != 0 && lastWidth != width) || (lastHeight != 0 && lastHeight != height)) {
            std::cout << "All textures given must be same size.\n";
            abort();
        }
        if (width != height && type == TEXTURE_CUBEMAP) {
            std::cout << "Cubemaps have to be square.\n";
            abort();
        }

        lastWidth = width;
        lastHeight = height;
    }

    unsigned int sourceFormat;
    switch (nChannels) {
    case 4:
    sourceFormat = RGBA;
    break;
    case 3:
    sourceFormat = RGB;
    break;
    case 1:
    sourceFormat = Grayscale;
    default:
    std::cout << "uh what the \n";
    abort();
    break;
    }

    // generate OpenGL texture object and put image data in it
    glGenTextures(1, &glTextureId);
    Use();
    if (type == TEXTURE_2D) {
        depth = 1;

        glTexImage3D(bindingLocation, 0, params.format, width, height, depth, 0, sourceFormat, GL_UNSIGNED_BYTE, imageDatas.back());
    }
    // else if (type == TEXTURE_2D_ARRAY) {
    //     if (layerHeight == -1) {layerHeight = height;}
    //     depth = height/layerHeight;
    //     height = layerHeight;
    //     glTexImage3D(bindingLocation, 0, GL_RGBA8, width, height, depth, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    // }

    // free the data loaded by stbi
    for (auto & data: imageDatas) {
        stbi_image_free(data);
    }
    
    ConfigTexture();
}

// float Texture::AddLayer() {
//     abort();
// }

Texture::~Texture() {
    glDeleteTextures(1, &glTextureId);
}

void Texture::Use() {
    glActiveTexture(glTextureIndex);
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

// Texture::Texture(TextureType textureType, std::string path, int layerHeight, int mipmapLevels) {
//     // use stbi_image.h to load file
//     type = textureType;
//     bindingLocation = (textureType == TEXTURE_FONT) ? GL_TEXTURE_2D : textureType;
//     unsigned char* imageData = stbi_load(path.c_str(), &width, &height, &nChannels, 4);
//     if (stbi_failure_reason()) { // error check
//         std::printf("STBI failed to load %s because %s", path.c_str(), stbi_failure_reason());
//     }

//     // generate OpenGL texture object and put image data in it
//     glGenTextures(1, &glTextureId);
//     Use();
//     if (type == TEXTURE_2D) {
//         depth = 1;

//         glTexImage2D(bindingLocation, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
//     }
//     else if (type == TEXTURE_2D_ARRAY) {
//         if (layerHeight == -1) {layerHeight = height;}
//         depth = height/layerHeight;
//         height = layerHeight;
//         glTexImage3D(bindingLocation, 0, GL_RGBA8, width, height, depth, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
//     }

//     stbi_image_free(imageData);

//     ConfigTexture();
// }

// Texture::Texture(TextureType textureType, std::vector<std::string>& paths, int mipmapLevels) {
//     type = textureType;
//     bindingLocation = textureType;
//     assert(type == TEXTURE_2D_ARRAY || type == TEXTURE_CUBEMAP);

//     // load the requested files
//     std::vector<unsigned char*> imageDatas;
    
//     int x = -1;
//     int y = -1;
//     for (auto & path: paths) {
//         // use stbi_image.h to load file
//         int imageX, imageY;
//         unsigned char* imageData = stbi_load(path.c_str(), &imageX, &imageY, &nChannels, 4); 
//         imageDatas.push_back(imageData);
//         // make sure images are the same size
//         if ((x != -1 && imageX != x) || (y != -1 && imageY != y)) {
//             std::printf("images are not the same size");
//             abort();
//         }
//         else {
//             x = imageX;
//             y = imageY;
//         }

//         if (stbi_failure_reason()) { // error check
//             std::printf("STBI failed to load %s because %s", path.c_str(), stbi_failure_reason());
//         }
//     }
//     width = x;
//     height = y;
//     if (type == TEXTURE_CUBEMAP) {
//         depth = 1;
//     }
//     else {
//         depth = imageDatas.size();
//     }
    

//     // generate OpenGL texture object and put image data in it
//     glGenTextures(1, &glTextureId);
//     Use();

//     if (type != TEXTURE_CUBEMAP) {
//         glTexImage3D(bindingLocation, mipmapLevels, GL_RGBA, width, height, imageDatas.size(), 0, GL_RGBA, GL_UNSIGNED_INT, nullptr);
//     }
//     else {
//         assert(imageDatas.size() == 6);
//     }

//     unsigned int i = 0;
//     for (auto data: imageDatas) {
//         if (type == TEXTURE_2D_ARRAY) {
//             glTexSubImage3D(bindingLocation, 0, 0, 0, i, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
//         }
//         else if (type == TEXTURE_CUBEMAP) {
//             glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
//         }
//         stbi_image_free(data);
//         i++;
//     }

//     ConfigTexture();
// }