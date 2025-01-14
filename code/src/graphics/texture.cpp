#include "gengine.hpp"
#include "GL/glew.h"
#include "glm/ext.hpp"
#include <cstring>
#include <ft2build.h>
#include <new>
#include FT_FREETYPE_H
// #include "../../external_headers/freetype/freetype/freetype.h"
#include "debug/assert.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "stb_image.h" 
#include "debug/debug.hpp"
#include "texture.hpp"
#include "framebuffer.hpp"
#include <tuple>
#include "assimp/scene.h"
#include <algorithm>

GLenum TextureBindingLocationFromType(Texture::TextureType type) {
    switch (type) {
    case Texture::Texture2D:
    return GL_TEXTURE_2D_ARRAY;    
    break;
    case Texture::TextureCubemap:
    return GL_TEXTURE_CUBE_MAP;
    break;
    case Texture::Texture2DFlat:
    return GL_TEXTURE_2D;
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
    DebugLogInfo("Writing new texture.");

    // skyboxes need 6 textures, everything else obviously only needs 1
    if (textureType == Texture::TextureCubemap) {
        Assert(params.textureSources.size() == 6);
    }
    else {
        Assert(params.textureSources.size() == 1);
    }

    

    // Get all the image data
    std::vector<std::shared_ptr<Image>> imageDatas; 

    if (usage != Texture::FontMap) { // For textures that aren't a font, we use stbi_image.h to load the files and then figure all the formatting and what not.

        int isArgb = -1; // -1 if unknown atm, 0 if false, 1 if true. If one image is argb, all of them must be.
        int lastWidth = 0, lastHeight = 0, lastNChannels = 0;
        for (auto & src: params.textureSources) {

            if (std::holds_alternative<std::shared_ptr<Image>>(src.imageData)) {
                imageDatas.push_back(std::get<std::shared_ptr<Image>>(src.imageData));
                Assert(imageDatas.back()->imageOrigin == Image::UserOrAssimpSupplied);
            }
            else if (std::holds_alternative<std::string>(src.imageData)) {
                std::string path = std::get<std::string>(src.imageData);

                const aiTexture* embeddedTexture = nullptr;
                if (params.scene != nullptr) { // then the image might have been embedded in a model file and then loaded by assimp
                    embeddedTexture = params.scene->GetEmbeddedTexture(path.c_str());
                }
                if (embeddedTexture == nullptr) { // then there is no embedded texture from assimp, we just use stbi_image.h to load the image file normally.

                    if (isArgb == 1) {
                        throw std::runtime_error("Mixed usage of embedded uncompressed argb textures and non-argb textures is illegal.");
                    }

                    // use stbi_image.h to load file
                    auto ptr = stbi_load(path.c_str(), &width, &height, &nChannels, NChannelsFromFormat(params.format));

                    // Check for stbi errors
                    // TODO: good design would be different exception classes for each loading error case
                    if (ptr == nullptr) {
                        throw std::runtime_error(std::string("STBI failed to load") + path + "because " + stbi_failure_reason() + ".");
                    }

                    imageDatas.emplace_back(std::make_shared<Image>(ptr, width, height, nChannels, Image::StbiSupplied));
                    // TODO: we don't throw an error if we put in a one channel image (for example) and wanted 3 channels, we just create 2 more channels, is that ok?
                    
                    nChannels = std::max(nChannels, (int)NChannelsFromFormat(params.format)); // apparently nChannels is set to the amount that the original image had, even if we asked for (and recieved) extra channels, so this makes sure it has the right value            
                }
                else { // assimp embedded a texture.
                    if (embeddedTexture->mHeight == 0) { // then the image is compressed (png, jpeg, etc.)

                        if (isArgb == 1) {
                            throw std::runtime_error("Mixed usage of embedded uncompressed argb textures and non-argb textures is illegal.");
                        }
                        isArgb = 0;

                        // in this case, we actually DO use stbi, which can load directly from the pointer provided by assimp, and we push the result of that into imageDats
                        // stbi even determines file format which is nice
                        auto ptr = stbi_load_from_memory((const stbi_uc*)(const void*)embeddedTexture->pcData, embeddedTexture->mWidth, &width, &height, &nChannels, NChannelsFromFormat(params.format));
                        imageDatas.emplace_back(std::make_shared<Image>(ptr, width, height, nChannels, Image::StbiSupplied));

                        // stbi error checking
                        // TODO: good design would be different exception classes for each of these cases
                        if (ptr == nullptr) {
                            throw std::runtime_error(std::string("STBI failed to load") + path + "because " + stbi_failure_reason() + ".");
                        }
                    }
                    else { // the image is not compressed, we simply need to determine numchannels/etc. and push back image buffer pointer into imageDatas.
                        // TODO: assimp docs have conflicting info on whether it argb or rgba, so for now its rgba and we'll fix it if it crashes later.
                        DebugLogInfo("LOADING UNCOMPRESSED FROM ASSIMP OMG ILL BE SHOCKED IF THIS WORKS");

                        width = embeddedTexture->mWidth;
                        height = embeddedTexture->mHeight;
                        nChannels = 4;

                        if (isArgb == 0) {}
                            throw std::runtime_error("Mixed usage of embedded uncompressed argb textures and non-argb textures is illegal.");
                        }
                        isArgb = 1; // assimp always gives uncompressed in argb according to docs
                        imageDatas.emplace_back(std::make_shared<Image>((uint8_t*)(embeddedTexture->pcData), width, height, nChannels, Image::UserOrAssimpSupplied));

                        DebugLogInfo("OMG IT WORKED U CAN DELETE THIS PRINT NOW");
                    }

            }
  
            // error checking
            // TODO: good design would be different exception classes for each of these cases
            if ((lastWidth != 0 && lastWidth != width) || (lastHeight != 0 && lastHeight != height)) {
                throw std::runtime_error("The given texture files had different image sizes. Cubemaps/multilayer textures must have the same size for every image/layer.");
            }
            if (width != height && type == Texture::TextureCubemap) {
                std::string path = std::holds_alternative<std::string>(src.imageData) ? std::get<std::string>(src.imageData) : "(from memory, not a file)";
                throw std::runtime_error(std::string("A cubemap texture was requested, but one of the given images \"") + path + "\" did not contain a square image (" + std::to_string(width) + " x " + std::to_string(height) + ").");
            }
            if (lastNChannels != 0 && lastNChannels != nChannels) {
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

        // if they asked for automatic format selection, pick one based on the number of channels we got.
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

        // setup wrapping, mipmaps, etc.
        ConfigTexture(params);

        // glBindTexture(bindingLocation, glTextureId);
        if (type == Texture::Texture2D) {
            depth = 1; 
            // glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // TODO: this may be neccesary in certain situations??? further investigation neededd
            // glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            // glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
            // glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
            void* data = imageDatas.back()->imageData;
            glTexImage3D(bindingLocation, 0, internalFormat, width, height, depth, 0, sourceFormat, GL_UNSIGNED_BYTE, data); // put data in opengl

            if (params.mipmapBehaviour != TextureMipmapBehaviour::NoMipmaps && params.mipmapGenerationMethod != TextureMipmapGeneration::GlGenerate) {
                GenMipmap(params, imageDatas.back()->imageData, internalFormat, sourceFormat, nChannels);
            }
            
        }
        else if (type == Texture::TextureCubemap) {
            depth = 1; 
            for (unsigned int i = 0; i < 6; i++) {
                void* data = imageDatas.at(i)->imageData;
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height, 0, sourceFormat, GL_UNSIGNED_BYTE, data);
                
                if (params.mipmapBehaviour != TextureMipmapBehaviour::NoMipmaps && params.mipmapGenerationMethod != TextureMipmapGeneration::GlGenerate) {
                    GenMipmap(params, imageDatas.at(i)->imageData, internalFormat, sourceFormat, nChannels);
                }
            }
        }
        else {
            Assert(false); // unreachable
        }
    
        if (params.mipmapBehaviour != TextureMipmapBehaviour::NoMipmaps && params.mipmapGenerationMethod == TextureMipmapGeneration::GlGenerate) {
            GenMipmap(params, nullptr, internalFormat, sourceFormat, nChannels);
        }

    }
    else { // to create font textures, we use freetype to rasterize them for us from vector ttf fonts
        Assert(params.format == TextureFormat::Grayscale_8Bit);
        Assert(std::holds_alternative<std::string>(params.textureSources.back().imageData));
        std::string fontPath = std::get<std::string>(params.textureSources.back().imageData);
        // TODO: optimization needed probably
        // TODO: apparently theres an stb_freetype library that might be better suited for this

        // make sure freetype libraryexists and was initialized successfully.
        FT_Library ft;
        Assert(!FT_Init_FreeType(&ft));
        
        // create a face (what freetype calls a loaded font)
        FT_Face face;
        Assert(!FT_New_Face(ft, fontPath.c_str(), 0, &face));

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
                DebugLogError("Freetype failed to load character ", c, " from font ", fontPath, "!");
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

        if (params.mipmapBehaviour != TextureMipmapBehaviour::NoMipmaps) {
            if (params.mipmapGenerationMethod != TextureMipmapGeneration::GlGenerate) {
                Assert(false); // TODO
                //GenMipmap(params, imageDatas.at(i)->imageData, internalFormat, sourceFormat, nChannels);
            }
            else {
                GenMipmap(params, nullptr, TextureFormat::Grayscale_8Bit, GL_RED, nChannels);
            }
        }
        

        // delete the glyph image data ptrs
        // TODO LITERAL MEMORY LEAK

        // tell freetype it can delete all its data now
        FT_Done_Face(face);
        FT_Done_FreeType(ft); // TODO: WAIT WE SHOULDN'T INIT AND UNINIT FREETYPE EVERY TIME WE MAKE A FONT

        // setup wrapping, mipmaps, etc.
        ConfigTexture(params);
    }
    
    

    
    
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
    Assert(params.textureSources.size() == 0); 
    //if (framebufferAttachmentType != GL_COLOR_ATTACHMENT0) return;

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
        // allocate storage for texture by passing nullptr as the data to load into the texture
    }
    else if (bindingLocation == GL_TEXTURE_2D) {
        //if (framebufferAttachmentType == GL_COLOR_ATTACHMENT0) {
            glTexImage2D(bindingLocation, 0, params.format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            ConfigTexture(params);
            //DebugLogInfo("ATTACHING ", framebufferAttachmentType, " TO ", framebuffer.glFramebufferId);

            glFramebufferTexture(framebuffer.bindingLocation, framebufferAttachmentType, glTextureId, 0);
        //}
    }
    else {
        std::cout << " add support first my guy\n";
        abort();
    }

    // Framebuffer::Unbind(); // TODO: probably not really needed and might carry high perf cost?
}

Texture::Texture(Texture&& old) :
    format(old.format),
    type(old.type),
    usage(old.usage),
    lineSpacing(old.lineSpacing),
    bindingLocation(old.bindingLocation),
    glTextureIndex(old.glTextureIndex),
    glTextureId(old.glTextureId),
    width(old.width),
    height(old.height),
    depth(old.depth),
    nChannels(old.nChannels),
    nMipmapLevels(old.nMipmapLevels),
    fontGlyphs(old.fontGlyphs)
{
    old.glTextureId = 0;
}

Texture::~Texture() {
    if (glTextureId != 0) { // could be 0 in case of move constructor
        glDeleteTextures(1, &glTextureId);
    }
    
}

glm::uvec3 Texture::GetSize() {
    return glm::uvec3(width, height, depth);
}

void Texture::Use() {
    glActiveTexture(GL_TEXTURE0 + glTextureIndex);
    //glBindTextureUnit(GL_TEXTURE0 + glTextureIndex, textureId); // TODOD: opengl 4.5 only
    glBindTexture(bindingLocation, glTextureId);
}

// for mipmapping
uint8_t Sample(uint8_t* pixels, int width, int height, int nChannels, Texture::TextureWrappingBehaviour wrapping, int x, int y, int channel) {
    //if (x >= width) {
    //    return 0;
    //    if (wrapping == Texture::WrapClampToEdge) {
    //        x = width - 1;
    //    }
    //    else if (wrapping == Texture::WrapTiled) {
    //        while (x >= width) x -= width;
    //    }
    //    else {
    //        Assert(false); // TODO, currently unsupported
    //    }
    //}

    //if (x < 0) {
    //    return 0;
    //    if (wrapping == Texture::WrapClampToEdge) {
    //        x = 0;
    //    }
    //    else if (wrapping == Texture::WrapTiled) {
    //        while (x < 0) x += width;
    //    }
    //    else {
    //        Assert(false); // TODO, currently unsupported
    //    }
    //}

    //if (y >= height) {
    //    return 0;
    //    if (wrapping == Texture::WrapClampToEdge) {
    //        y = height - 1;
    //    }
    //    else if (wrapping == Texture::WrapTiled) {
    //        while (y >= height) y -= height;
    //    }
    //    else {
    //        Assert(false); // TODO, currently unsupported
    //    }
    //}

    //if (y < 0) {
    //    return 0;
    //    if (wrapping == Texture::WrapClampToEdge) {
    //        y = 0;
    //    }
    //    else if (wrapping == Texture::WrapTiled) {
    //        while (y < 0) y += height;
    //    }
    //    else {
    //        Assert(false); // TODO, currently unsupported
    //    }
    //}

    //if (y / 4 % 2 == 0) {
    //    return 0;
    //}
    //
    //return 255;
    return pixels[x * nChannels * height + y * nChannels + channel];
}

void Texture::GenMipmap(const TextureCreateParams& params, uint8_t* src, TextureFormat internalFormat, unsigned int sourceFormat, unsigned int nChannels, int face, int level) {
    Assert(params.mipmapBehaviour != NoMipmaps);

    //DebugLogInfo("Mipmapping ", textureId, " face ", face, " level ", level);

    if (params.mipmapGenerationMethod == GlGenerate) {
       
        glGenerateMipmap(bindingLocation);
    }
    else {
        nMipmapLevels++;

        // confirm that texture dimensions are a power of two
        if ((width & (width - 1)) != 0 || (height & (height - 1)) != 0) {
            throw std::runtime_error("Invalid texture dimensions for automatic mipmap generation by AG3 (must be power of two).");
        }

        int mipWidth = width / (pow(2, level));
        int mipHeight = height / (pow(2, level));

        // generate mipmap
        uint8_t* dst = new uint8_t[mipWidth * mipHeight * nChannels];
        int tx = 0;
        int ty = 0;
        int stride = 2;
        for (int x = 0; x < mipWidth; x++) {
            for (int y = 0; y < mipHeight; y++) {
                for (int channel = 0; channel < nChannels; channel++) {
                    float sum = 0;

                    sum += Sample(src, mipWidth, mipHeight, nChannels, params.wrappingBehaviour, tx,     ty, channel);
                    //sum += Sample(src, mipWidth, mipHeight, nChannels, params.wrappingBehaviour, tx + 1, ty, channel);
                    //sum += Sample(src, mipWidth, mipHeight, nChannels, params.wrappingBehaviour, tx    , ty + 1, channel);
                    //sum += Sample(src, mipWidth, mipHeight, nChannels, params.wrappingBehaviour, tx + 1, ty + 1, channel);

                   /* sum += Sample(src, mipWidth, mipHeight, nChannels, params.wrappingBehaviour, x - 1, y - 1, channel);
                    sum += Sample(src, mipWidth, mipHeight, nChannels, params.wrappingBehaviour, x + 0, y - 1, channel);
                    sum += Sample(src, mipWidth, mipHeight, nChannels, params.wrappingBehaviour, x + 1, y - 1, channel);
                    sum += Sample(src, mipWidth, mipHeight, nChannels, params.wrappingBehaviour, x - 1, y + 0, channel);
                    sum += Sample(src, mipWidth, mipHeight, nChannels, params.wrappingBehaviour, x + 0, y + 0, channel);
                    sum += Sample(src, mipWidth, mipHeight, nChannels, params.wrappingBehaviour, x + 1, y + 0, channel);
                    sum += Sample(src, mipWidth, mipHeight, nChannels, params.wrappingBehaviour, x - 1, y + 1, channel);
                    sum += Sample(src, mipWidth, mipHeight, nChannels, params.wrappingBehaviour, x + 0, y + 1, channel);
                    sum += Sample(src, mipWidth, mipHeight, nChannels, params.wrappingBehaviour, x + 1, y + 1, channel);*/

                    //DebugLogInfo("Sample for channel ", channel, " is ", sum);

                    //sum /= 4;

                    //sum = (x % 8 > 3) * 255;

                    dst[x * nChannels * mipHeight + y * nChannels + channel] = sum;
                }
                ty += stride;
            }
            tx += stride;
        }
        
        // give OpenGL the mipmap
        if (type == TextureType::Texture2D) {
            glTexImage3D(bindingLocation, level, internalFormat, mipWidth, mipHeight, depth, 0, sourceFormat, GL_UNSIGNED_BYTE, dst); // put data in opengl
        }
        else if (type == TextureType::TextureCubemap) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level, internalFormat, mipWidth, mipHeight, 0, sourceFormat, GL_UNSIGNED_BYTE, dst);
        }
        else {
            Assert(false); // unreachable
        }
        // generate next mipmap level unless this is the last one
        if (mipWidth != 1 && mipHeight != 1) { // TODO: technically not the right condition
            GenMipmap(params, dst, internalFormat, sourceFormat, nChannels, face, level + 1);
        }

        delete[] dst;
    }
}

// Sets all of the OpenGL texture parameters.
void Texture::ConfigTexture(const TextureCreateParams& params) {
    // mag filter vs min filter:
    // mag filter is used when a fragment (pixel) on the screen is smaller than the pixels on texture (for close up objects)
    // min filter is used when each fragment covers up many pixels on a texture (for far away objects)

    Use();

    if (nMipmapLevels != 0) {
        glTexParameteri(bindingLocation, GL_TEXTURE_MAX_LEVEL, nMipmapLevels);
    }

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
        DebugLogError("something very wrong in config texture filtering"); abort();
        }


    break;
    case Texture::LinearMipmapInterpolation:
    
        switch (params.filteringBehaviour) {
        case Texture::LinearTextureFiltering:
        glTexParameteri(bindingLocation, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        break;
        case Texture::NoTextureFiltering:
        glTexParameteri(bindingLocation, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR); 
        break;
        default:
        DebugLogError("something very wrong in config texture filtering"); abort();
        }
    

    break;
    default:
    DebugLogError("something very wrong in config texture mipmapping"); abort();
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
    DebugLogError("something very wrong in config texture wrapping"); abort();
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
    DebugLogError("something very wrong in config texture filtering"); abort();
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

TextureCreateParams::TextureCreateParams(const std::vector<TextureSource>& imagePaths, const Texture::TextureUsage texUsage):
    textureSources(imagePaths),
    usage(texUsage)
{
    format = Texture::Auto_8Bit;

    wrappingBehaviour = Texture::WrapTiled;
    mipmapBehaviour = Texture::LinearMipmapInterpolation;
    filteringBehaviour = Texture::LinearTextureFiltering;
    mipmapGenerationMethod = Texture::GlGenerate;
    
    fontHeight = 48;
}

TextureSource::TextureSource(std::string imagePath): imageData(imagePath)
{
}

TextureSource::TextureSource(std::shared_ptr<Image> image) : imageData(image)
{
}

//Image TextureSource::GetImage()
//{
    //if (std::holds_alternative<Image>(imageData)) {
        //return std::get<Image>(imageData);
    //}
    //else {
        //std::string path = std::get<std::string>(imageData);

        //return Image{
        //.format = format,
        //.imageData = data,
        //.len = len
        //}
    //}
    
//}

Image::Image(uint8_t* data, int width, int height, int nChannels, ImageOrigin origin):
    imageData(data),
    width(width),
    height(height),
    nChannels(nChannels),
    //format(format),
    imageOrigin(origin)
{
}

//Image::Image(Image&& old)
//{
//    imageData = old.imageData;
//    imageOrigin = old.imageOrigin;
//    width = old.width;
//    height = old.height;
//    nChannels = old.nChannels;
//    //format = old.format;
//
//    old.imageData = nullptr;
//}

Image::~Image()
{
    if (imageData != nullptr && imageOrigin == Image::StbiSupplied) {
        stbi_image_free(imageData);
    }
}
