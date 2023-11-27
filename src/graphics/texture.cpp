#include "engine.hpp"
#include "../../external_headers/GLEW/glew.h"
#include "../../external_headers/GLM/ext.hpp"
#include "../../external_headers/freetype/ft2build.h"
#include <algorithm>
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
    return STBI_rgb_alpha; // same as 4
    break;
    case RGB:
    return STBI_rgb; // same as 3
    break;
    case Grayscale:
    return STBI_grey; // same as 1
    default:
    std::cout << "forgot something\n";
    abort();
    break;
    }
    return 0; // gotta return something to hide the warning
}

// Create texture.
Texture::Texture(const TextureCreateParams& params, const GLuint textureIndex, TextureType textureType):
format(params.format),
bindingLocation(TextureBindingLocationFromType(textureType)),
glTextureIndex(textureIndex),
type(textureType)
{
    // skyboxes need 6 textures, everything else obviously only needs 1
    if (textureType == TEXTURE_CUBEMAP) {
        assert(params.texturePaths.size() == 6);
    }
    else {
        assert(params.texturePaths.size() == 1);
    }

    // Get all the image data
    std::vector<unsigned char*> imageDatas;
    
    // std::cout << "Requiring " << NChannelsFromFormat(params.format) << " channels.\n";
    unsigned int lastWidth = 0, lastHeight = 0;
    for (auto & path: params.texturePaths) {
        // use stbi_image.h to load file
        imageDatas.push_back(stbi_load(path.c_str(), &width, &height, &nChannels, NChannelsFromFormat(params.format)));
        nChannels = std::max(nChannels, (int)NChannelsFromFormat(params.format)); // apparently nChannels is set to the amount that the original image had, even if we asked for (and recieved) extra channels, so this makes sure it has the right value
        if (imageDatas.back() == nullptr) { // error check
            std::printf("STBI failed to load %s because %s", path.c_str(), stbi_failure_reason());
            abort(); // TODO: set image data to pointer to an error texture when this happens instead of aborting
        }
        if ((lastWidth != 0 && lastWidth != width) || (lastHeight != 0 && lastHeight != height)) {
            std::cout << "All textures given must be same size.\n";
            abort();
        }
        if (width != height && type == TEXTURE_CUBEMAP) {
            std::cout << "Cubemaps have to be square.\n";
            abort();
        }
        // TODO: we should also assert that all files have same number of color/alpha channels

        lastWidth = width;
        lastHeight = height;
    }
    std::cout << "Got " << nChannels << " channels.\n";
    std::printf("Width %u Height %u \n", width, height);
    // std::cout << "Texture data: ";
    // for (unsigned int i = 0; i < width * height; i++) {
    //     std:: cout << (unsigned int)(imageDatas.back()[i]) << ", ";
    // }
    std::cout << "\n";
    // Determine what format the image data was in; RGB? RGBA? etc (nChannels is how many components the loaded mage had)
    unsigned int sourceFormat;
    switch (nChannels) {
    case 4:
    sourceFormat = GL_RGBA;
    break;
    case 3:
    sourceFormat = GL_RGB;
    break;
    case 1:
    sourceFormat = GL_RED;
    break;
    default:
    std::cout << "uh what the \n";
    abort();
    break;
    }

    // generate OpenGL texture object and put image data in it
    glGenTextures(1, &glTextureId);
    // Use();
    glBindTexture(bindingLocation, glTextureId);
    if (type == TEXTURE_2D) {
        depth = 1;
        //glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // TODO: this may be neccesary in certain situations??? further investigation neededd
        glTexImage3D(bindingLocation, 0, params.format, width, height, depth, 0, sourceFormat, GL_UNSIGNED_BYTE, imageDatas.back()); // put data in opengl
    }
    else {
        std::cout << " you forgot cubemap texture constructor my guy"; abort();
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
    glActiveTexture(GL_TEXTURE0 + glTextureIndex);
    //glBindTextureUnit(GL_TEXTURE0 + glTextureIndex, textureId); // TODOD: opengl 4.5 only
    glBindTexture(bindingLocation, glTextureId);
}

// Sets all of the OpenGL texture parameters.
void Texture::ConfigTexture() {
    Use();
    glTexParameteri(bindingLocation, GL_TEXTURE_WRAP_S, GL_REPEAT); // GL_REPEAT means tile the texture; TODO: add params for this stuff
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