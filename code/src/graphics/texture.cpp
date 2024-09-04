#include "gengine.hpp"
#include "GL/glew.h"
#include "glm/ext.hpp"
#include <cstring>
#include <ft2build.h>
#include <new>
#include <variant>
#include FT_FREETYPE_H
// #include "../../external_headers/freetype/freetype/freetype.h"
#include <algorithm>
#include "debug/assert.hpp"
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <process.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "stb_image.h" 
#include "debug/debug.hpp"
#include "texture.hpp"
#include "framebuffer.hpp"

#include "assimp/scene.h"


GLenum TextureBindingLocationFromType(Texture::TextureType type) {
    switch (type) {
    case Texture::Texture2D:
    return GL_TEXTURE_2D_ARRAY;    
    break;
    case Texture::TextureCubemap:
    return GL_TEXTURE_CUBE_MAP;
    break;
    default:
    std::cout << "forgot something\n";
    abort();
    
    break;
    }
    // return 0; // gotta return something to hide the warning
}

unsigned int NChannelsFromFormat(Texture::TextureFormat format ) {
    switch (format) {
    case Texture::RGBA_8Bit:
    return STBI_rgb_alpha; // same as 4
    break;
    case Texture::RGB_8Bit:
    return STBI_rgb; // same as 3
    break;
    case Texture::Grayscale_8Bit:
    return STBI_grey; // same as 1
    case Texture::Auto_8Bit:
    return 0; // 0 means we don't tell STBI to return a specific number of channels
    default:
    std::cout << "forgot something2\n";
    abort();
    break;
    }
    return 0; // gotta return something to hide the warning
}

// Create texture (for use on objects).
Texture::Texture(const TextureCreateParams& params, const GLuint textureIndex, const TextureType textureType):
format(params.format),
type(textureType),
usage(params.usage),
lineSpacing(params.fontHeight), // TODO: get line spacing from face->height instead
bindingLocation(TextureBindingLocationFromType(textureType)),
glTextureIndex(textureIndex)
{
    // skyboxes need 6 textures, everything else obviously only needs 1
    if (textureType == Texture::TextureCubemap) {
        Assert(params.texturePaths.size() == 6);
    }
    else {
        Assert(params.texturePaths.size() == 1);
    }

    enum ImageOrigin {
        UserOrAssimpSupplied = 0, // the user (or assimp) controls the image's data and we don't delete it or anything
        StbiSupplied = 1 // We created this image via stbi and we must delete it through stbi, too.
    };

    // Get all the image data
    // if unsigned char*, it's from stbi and we need to free it when this function is done. if aiTexel*, assimp owns it and it's not our problem.
    std::vector<std::pair<Image, ImageOrigin> imageDatas; 

    if (usage != Texture::FontMap) { // For textures that aren't a font, we use stbi_image.h to load the files and then figure all the formatting and what not.

        int isArgb = -1; // -1 if unknown atm, 0 if false, 1 if true. If one image is argb, all of them must be.
        int lastWidth = 0, lastHeight = 0, lastNChannels = 0;
        for (auto & path: params.texturePaths) {

            const aiTexture* embeddedTexture = nullptr;
            if (params.scene != nullptr ) { // then the image might have been embedded in a model file and then loaded by assimp
                embeddedTexture = params.scene->GetEmbeddedTexture(path.c_str());
            }
            if (embeddedTexture == nullptr) { // then there is no embedded texture from assimp, we just use stbi_image.h to load the image file normally.
                
                if (isArgb == 1) {
                    // because stbi doesn't use RAII like a cool person we have to manually free the texture data even when it errors.
                    for (auto & data: imageDatas) {
                        if (std::holds_alternative<unsigned char*>(data)) {
                            stbi_image_free(std::get<unsigned char*>(data));
                        }  
                    }
                    throw std::runtime_error("Mixed usage of embedded uncompressed argb textures and non-argb textures is illegal.");  
                }

                // use stbi_image.h to load file
                imageDatas.push_back(stbi_load(path.c_str(), &width, &height, &nChannels, NChannelsFromFormat(params.format)));
                // TODO: we don't throw an error if we put in a one channel image (for example) and wanted 3 channels, we just create 2 more channels, is that ok?
                nChannels = std::max(nChannels, (int)NChannelsFromFormat(params.format)); // apparently nChannels is set to the amount that the original image had, even if we asked for (and recieved) extra channels, so this makes sure it has the right value
           
                // error checking
                // TODO: good design would be different exception classes for each of these cases
                if (std::get<unsigned char*>(imageDatas.back()) == nullptr) { 
                    imageDatas.pop_back(); // don't delete nullptr

                    // because stbi doesn't use RAII like a cool person we have to manually free the texture data even when it errors.
                    for (auto & data: imageDatas) {
                        if (std::holds_alternative<unsigned char*>(data)) {
                            stbi_image_free(std::get<unsigned char*>(data));
                        }  
                    }
                    throw std::runtime_error(std::string("STBI failed to load") + path + "because " + stbi_failure_reason() + ".");  
                }
            }
            else { // assimp embedded a texture.
                if (embeddedTexture->mHeight == 0) { // then the image is compressed (png, jpeg, etc.)

                    if (isArgb == 1) {
                        // because stbi doesn't use RAII like a cool person we have to manually free the texture data even when it errors.
                        for (auto & data: imageDatas) {
                            if (std::holds_alternative<unsigned char*>(data)) {
                                stbi_image_free(std::get<unsigned char*>(data));
                            }  
                        }
                        throw std::runtime_error("Mixed usage of embedded uncompressed argb textures and non-argb textures is illegal.");  
                    }

                    isArgb = 0;
                    
                    // in this case, we actually DO use stbi, which can load directly from the pointer provided by assimp, and we push the result of that into imageDats
                    // stbi even determines file format which is nice
                    imageDatas.push_back(stbi_load_from_memory((const stbi_uc*)(const void*)embeddedTexture->pcData, embeddedTexture->mWidth, &width, &height, &nChannels, NChannelsFromFormat(params.format)));

                    // error checking
                    // TODO: good design would be different exception classes for each of these cases
                    if (std::get<unsigned char*>(imageDatas.back()) == nullptr) { 
                        imageDatas.pop_back(); // don't delete nullptr

                        // because stbi doesn't use RAII like a cool person we have to manually free the texture data even when it errors.
                        for (auto & data: imageDatas) {
                            if (std::holds_alternative<unsigned char*>(data)) {
                                stbi_image_free(std::get<unsigned char*>(data));
                            }  
                        }
                        throw std::runtime_error(std::string("STBI failed to load") + path + "because " + stbi_failure_reason() + ".");  
                    }
                }
                else { // the image is not compressed, we simply need to determine numchannels/etc. and push back image buffer pointer into imageDatas.
                    // TODO: assimp docs have conflicting info on whether it argb or rgba, so for now its rgba and we'll fix it if it crashes later.
                    DebugLogInfo("LOADING UNCOMPRESSED FROM ASSIMP OMG ILL BE SHOCKED IF THIS WORKS");

                    width = embeddedTexture->mWidth;
                    height = embeddedTexture->mHeight;
                    nChannels = 4;

                    if (isArgb == 0) {
                        // because stbi doesn't use RAII like a cool person we have to manually free the texture data even when it errors.
                        for (auto & data: imageDatas) {
                            if (std::holds_alternative<unsigned char*>(data)) {
                                stbi_image_free(std::get<unsigned char*>(data));
                            }  
                        }
                        throw std::runtime_error("Mixed usage of embedded uncompressed argb textures and non-argb textures is illegal.");  
                    }
                    isArgb = 1; // assimp always gives uncompressed in argb according to docs
                    imageDatas.push_back(embeddedTexture->pcData);

                    DebugLogInfo("OMG IT WORKED U CAN DELETE THIS PRINT NOW");
                }
                
            }
            
            // error checking
            // TODO: good design would be different exception classes for each of these cases
            if ((lastWidth != 0 && lastWidth != width) || (lastHeight != 0 && lastHeight != height)) {
                // because stbi doesn't use RAII like a cool person we have to manually free the texture data even when it errors.
                for (auto & data: imageDatas) {
                    if (std::holds_alternative<unsigned char*>(data)) {
                        stbi_image_free(std::get<unsigned char*>(data));
                    } 
                }
                throw std::runtime_error("The given texture files had different image sizes. Cubemaps/multilayer textures must have the same size for every image/layer.");
            }
            if (width != height && type == Texture::TextureCubemap) {
                // because stbi doesn't use RAII like a cool person we have to manually free the texture data even when it errors.
                for (auto & data: imageDatas) {
                    if (std::holds_alternative<unsigned char*>(data)) {
                        stbi_image_free(std::get<unsigned char*>(data));
                    }  
                }
                throw std::runtime_error(std::string("A cubemap texture was requested, but one of the given images \"") + path + "\" did not contain a square image (" + std::to_string(width) + " x " + std::to_string(height) + ").");
            }
            if (lastNChannels != 0 && lastNChannels != nChannels) {
                // because stbi doesn't use RAII like a cool person we have to manually free the texture data even when it errors.
                for (auto & data: imageDatas) {
                    if (std::holds_alternative<unsigned char*>(data)) {
                        stbi_image_free(std::get<unsigned char*>(data));
                    }  
                }
                throw std::runtime_error("The given texture files had different numbers of channels. Cubemaps/multilayer textures must have the same number of channels for every image/layer.");
            }

            lastWidth = width;
            lastHeight = height;
            lastNChannels = nChannels;
        }

        // Determine what format the image data was in; RGB? RGBA? etc (nChannels is how many components the loaded mage had)
        // TODO: bug related to # of channels in source image being too high 
        unsigned int sourceFormat;
        switch (nChannels) {
        case 4:
        sourceFormat = GL_RGBA; //std::cout << "src picked rgba\n";
        break;
        case 3:
        sourceFormat = GL_RGB;// std::cout << "src picked rgb\n";
        break;
        case 1:
        sourceFormat = GL_RED; //std::cout << "src picked grayscale\n";
        break;
        default:
        Assert(false); // unreachable
        break;
        }

        TextureFormat internalFormat = params.format;
        if (params.format == Texture::Auto_8Bit) {
            switch (nChannels) {
            case 4:
            internalFormat = Texture::RGBA_8Bit; //std::cout << "auto picked rgba\n";
            break;
            case 3:
            internalFormat = Texture::RGB_8Bit;// std::cout << "auto picked rgb\n";
            break;
            case 1:
            internalFormat = Texture::Grayscale_8Bit;// std::cout << "auto picked grayscale\n";
            break;
            default:
            Assert(false); // unreachable
            break;
            }
        } 

        // TODO: there was a crash when loading a 3-channel jpeg to create a grayscale texture
        // generate OpenGL texture object and put image data in it
        glGenTextures(1, &glTextureId);

        // std::cout << "Textuhhuhuhuhuure data: ";
        // for (unsigned int i = 0; i < width; i++) {
        //     std:: cout << (int)(imageDatas.back()[i]) << " ";
        // }
        Use();
        // glBindTexture(bindingLocation, glTextureId);
        if (type == Texture::Texture2D) {
            depth = 1; 
            // glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // TODO: this may be neccesary in certain situations??? further investigation neededd
            // glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            // glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
            // glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
            void* data;
            if (std::holds_alternative<unsigned char*>(imageDatas.back())) {
                data = std::get<unsigned char*>(imageDatas.back());
            }
            else {
                data = std::get<aiTexel*>(imageDatas.back());
            }
            glTexImage3D(bindingLocation, 0, internalFormat, width, height, depth, 0, sourceFormat, GL_UNSIGNED_BYTE, data); // put data in opengl
        }
        else if (type == Texture::TextureCubemap) {
            depth = 1; 
            for (unsigned int i = 0; i < 6; i++) {
                void* data;
                if (std::holds_alternative<unsigned char*>(imageDatas.at(i))) {
                    data = std::get<unsigned char*>(imageDatas.at(i));
                }
                else {
                    data = std::get<aiTexel*>(imageDatas.at(i));
                }
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height, 0, sourceFormat, GL_UNSIGNED_BYTE, data);
            }
            
        }
        // else if (type == TEXTURE_2D_ARRAY) {
        //     if (layerHeight == -1) {layerHeight = height;}
        //     depth = height/layerHeight;
        //     height = layerHeight;
        //     glTexImage3D(bindingLocation, 0, GL_RGBA8, width, height, depth, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
        // }
        else {
            Assert(false); // unreachable
        }

        // free the data loaded by stbi (but not the data loaded by assimp)
        for (auto & data: imageDatas) {
            if (std::holds_alternative<unsigned char*>(data)) {
                stbi_image_free(std::get<unsigned char*>(data));
            }  
        }
    
    }
    else { // to create font textures, we use freetype to rasterize them for us from vector ttf fonts
        Assert(params.format == TextureFormat::Grayscale_8Bit);

        // TODO: optimization needed probably
        // TODO: apparently theres an stb_freetype library that might be better suited for this

        // make sure freetype libraryexists and was initialized successfully.
        FT_Library ft;
        Assert(!FT_Init_FreeType(&ft));
        
        // create a face (what freetype calls a loaded font)
        FT_Face face;
        Assert(!FT_New_Face(ft, params.texturePaths.back().c_str(), 0, &face));

        // set font size
        Assert(params.fontHeight != 0);
        FT_Set_Pixel_Sizes(face, 0, params.fontHeight);

        // collect glyphs from font, and track information needed to determine size of OpenGL texture
        unsigned int totalWidth = 0;
        unsigned int greatestHeight = 0;

        fontGlyphs.emplace();
        std::unordered_map<char, void*> glyphImageDataPtrs;
        for (unsigned char c = 0; c < 128; c++) { // C++ NO WAYYYYYYYY!!!!
            if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            {
                std::cout << "ERROR::FREETYTPE: Failed to load character " << c << " from font " << params.texturePaths.back() << "/n";
                abort();
            }

            greatestHeight = std::max(greatestHeight, face->glyph->bitmap.rows + 1); // add 1 pixel of vertial space between each row of rasterized glyphs on the texture
            totalWidth += face->glyph->bitmap.width + 1; // add 1 pixel of horizontal space between each glyph on the texture

            (*fontGlyphs)[c] = Glyph {
                .width = face->glyph->bitmap.width,
                .height = face->glyph->bitmap.rows,
                .advance = (unsigned int)((float)(face->glyph->advance.x)/64.0f), // advance is for some reason given in the dumbest imaginable units so must be converted
                .bearingX = face->glyph->bitmap_left,
                .bearingY = face->glyph->bitmap_top,
                // UVs are done later
            };

            // if (c == '\'') {
            //     std::cout << "Character \' has height " << face->glyph->bitmap.rows << " and bearingY " << face->glyph->bitmap_top << ".\n";
            // }
            
            // when we call load_char for the next one, it will delete this char, so we just gonna memcpy it to a new void*
            void* newBuffer = operator new[](face->glyph->bitmap.width * face->glyph->bitmap.rows);
            memcpy(newBuffer, face->glyph->bitmap.buffer, face->glyph->bitmap.width * face->glyph->bitmap.rows);
            glyphImageDataPtrs[c] = newBuffer;
        }

        // create openGL texture
        glGenTextures(1, &glTextureId);
        Use();

        // allocate space for all the glyphs
        int maxWidth; // if texture is too wide to be supported by the OpenGL implmentation, we split it into multiple rows. 
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxWidth);
        unsigned int numRows = totalWidth / maxWidth + (totalWidth % maxWidth != 0);

        width = std::min(totalWidth, (unsigned int)maxWidth);
        height = numRows * greatestHeight;
        unsigned int layer = 0; // TODO: OPTIMIZE BY PUTTING DIFFERENT FONTS ON SAME FONTMAP
        
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R8, width, height, 1, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

        // fill texture with glyphs and calculate uvs
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // make sure we don't segfault
        int currentX = 0;
        int currentY = 0;
        for (auto & [character, glyph] : *fontGlyphs) {

            // see if there's room for another character on this row, and if not, move to the next row    
            if (int(currentX + glyph.width + 1) >= maxWidth) {
                currentY += greatestHeight;
                currentX = 0;
            }

            glyph.topUv = GLfloat(currentY)/GLfloat(height);
            glyph.leftUv = GLfloat(currentX)/GLfloat(width);

            glyph.bottomUv = GLfloat(currentY + glyph.height)/GLfloat(height);
            glyph.rightUv = GLfloat(currentX + glyph.width)/GLfloat(width);

            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,  currentX, currentY, layer, glyph.width, glyph.height, 1, GL_RED, GL_UNSIGNED_BYTE, glyphImageDataPtrs.at(character));
            currentX += (glyph.width + 1);

            
            
        };

        // delete the glyph image data ptrs
        // TODO LITERAL MEMORY LEAK

        // tell freetype it can delete all its data now
        FT_Done_Face(face);
        FT_Done_FreeType(ft); // TODO: WAIT WE SHOULDN'T INIT AND UNINIT FREETYPE EVERY TIME WE MAKE A FONT
    }
    
    // setup wrapping, mipmaps, etc.
    ConfigTexture(params);
    
}

// float Texture::AddLayer() {
//     abort();
// }

// Create texture and attach it to framebuffer
Texture::Texture(Framebuffer& framebuffer, const TextureCreateParams& params, const GLuint textureIndex, const TextureType textureType, const GLenum framebufferAttachmentType):
format(params.format),
type(textureType),
usage(params.usage),
lineSpacing(params.fontHeight),
bindingLocation(TextureBindingLocationFromType(textureType)),
glTextureIndex(textureIndex) 
{
    Assert(params.texturePaths.size() == 0); 

    glGenTextures(1, &glTextureId);
    Use();

    width = framebuffer.width;
    height = framebuffer.height;
    depth = 1; // TODO

    // std::cout << " bind buffer for tex creation\n";
    framebuffer.Bind(); // bind the framebuffer so we can attach textures to it
    if (bindingLocation == GL_TEXTURE_2D_ARRAY) {

        // allocate storage for texture by passing nullptr as the data to load into the texture
        glTexImage3D(bindingLocation, 0, params.format, width, height, depth, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        ConfigTexture(params);

        // attach the texture to the framebuffer
        glFramebufferTextureLayer(framebuffer.bindingLocation, framebufferAttachmentType, glTextureId, 0, 0);
    }
    else {
        std::cout << " add support first my guy\n";
        abort();
    }

    // Framebuffer::Unbind(); // TODO: probably not really needed and might carry high perf cost?
}

Texture::~Texture() {
    glDeleteTextures(1, &glTextureId);
}

void Texture::Use() {
    glActiveTexture(GL_TEXTURE0 + glTextureIndex);
    //glBindTextureUnit(GL_TEXTURE0 + glTextureIndex, textureId); // TODOD: opengl 4.5 only
    glBindTexture(bindingLocation, glTextureId);
}

// Sets all of the OpenGL texture parameters.
void Texture::ConfigTexture(const TextureCreateParams& params) {
    // mag filter vs min filter:
    // mag filter is used when a fragment (pixel) on the screen is smaller than the pixels on texture (for close up objects)
    // min filter is used when each fragment covers up many pixels on a texture (for far away objects)

    Use();

    // mipmapping and filtering settings used for determining min filter
    switch (params.mipmapBehaviour) {
    case Texture::NoMipmaps:

        switch (params.filteringBehaviour) {
        case Texture::LinearTextureFiltering:
        glTexParameteri(bindingLocation, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        break;
        case Texture::NoTextureFiltering:
        glTexParameteri(bindingLocation, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
        break;
        default:
        std::cout << "something very wrong in config texture filtering\n"; abort();
        }
    
    break;
    case Texture::UseNearestMipmap:

        switch (params.filteringBehaviour) {
        case Texture::LinearTextureFiltering:
        glTexParameteri(bindingLocation, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        break;
        case Texture::NoTextureFiltering:
        glTexParameteri(bindingLocation, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST); 
        break;
        default:
        std::cout << "something very wrong in config texture filtering\n"; abort();
        }

    glGenerateMipmap(bindingLocation); // make sure we actually have mipmaps to use in this case

    break;
    case Texture::LinearMipmapInterpolation:
    
        switch (params.filteringBehaviour) {
        case Texture::LinearTextureFiltering:
        glTexParameteri(bindingLocation, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        break;
        case Texture::NoTextureFiltering:
        glTexParameteri(bindingLocation, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST); 
        break;
        default:
        std::cout << "something very wrong in config texture filtering\n"; abort();
        }
    
    glGenerateMipmap(bindingLocation); // make sure we actually have mipmaps to use in this case

    break;
    default:
    std::cout << "something very wrong in config texture mipmapping\n"; abort();
    }

    // wrapping settings
    switch (params.wrappingBehaviour) {
    case Texture::WrapClampToEdge:
    glTexParameteri(bindingLocation, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
    glTexParameteri(bindingLocation, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (glTextureIndex == GL_TEXTURE_CUBE_MAP) {glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE); }
    break;
    case Texture::WrapTiled:
    glTexParameteri(bindingLocation, GL_TEXTURE_WRAP_S, GL_REPEAT); // GL_REPEAT means tile the texture
    glTexParameteri(bindingLocation, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if (glTextureIndex == GL_TEXTURE_CUBE_MAP) {glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_REPEAT); }
    break;
    case Texture::WrapMirroredTiled:
    glTexParameteri(bindingLocation, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT); 
    glTexParameteri(bindingLocation, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    if (glTextureIndex == GL_TEXTURE_CUBE_MAP) {glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_MIRRORED_REPEAT); }
    break;
    case Texture::WrapMirroredClamped:
    glTexParameteri(bindingLocation, GL_TEXTURE_WRAP_S, GL_MIRROR_CLAMP_TO_EDGE); 
    glTexParameteri(bindingLocation, GL_TEXTURE_WRAP_T, GL_MIRROR_CLAMP_TO_EDGE);
    if (glTextureIndex == GL_TEXTURE_CUBE_MAP) {glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_MIRROR_CLAMP_TO_EDGE); }
    break;
    default:
    std::cout << "something very wrong in config texture wrappnig\n"; abort();
    }

    // filtering settings again for the mag filter
    // TODO: literaaly every other setting???
    switch (params.filteringBehaviour) {
    case Texture::LinearTextureFiltering:
    glTexParameteri(bindingLocation, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    break;
    case Texture::NoTextureFiltering:
    glTexParameteri(bindingLocation, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // nearest means choose the closest pixel instead of interpolating between multiple
    break;
    default:
    std::cout << "something very wrong in config texture filtering\n"; abort();
    }
    
    
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
//     Assert(type == TEXTURE_2D_ARRAY || type == TEXTURE_CUBEMAP);

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
//         Assert(imageDatas.size() == 6);
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

TextureCreateParams::TextureCreateParams(const std::vector<std::string>& imagePaths, const Texture::TextureUsage texUsage):
    texturePaths(imagePaths),
    usage(texUsage)
{
    format = Texture::Auto_8Bit;

    wrappingBehaviour = Texture::WrapTiled;
    mipmapBehaviour = Texture::LinearMipmapInterpolation;
    filteringBehaviour = Texture::LinearTextureFiltering;
    
    fontHeight = 48;
}

TextureSource::TextureSource(std::string imagePath): imageData(imagePath)
{
}

TextureSource::TextureSource(Image image): imageData(image)
{
}

Image TextureSource::GetImage()
{
    return Image{
        .format = format,
        .imageData = data,
        .len = len
    }
}
