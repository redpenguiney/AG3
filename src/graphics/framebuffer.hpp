#pragma once
#include "../../external_headers/GLEW/glew.h"
#include <optional>
#include <vector>
#include "texture.hpp"

// A framebuffer in OpenGL 
class Framebuffer {
    public:
    Framebuffer(const unsigned int fbWidth, const unsigned int fbHeight, const std::vector<TextureCreateParams>& attachmentParams, const bool haveDepthRenderbuffer);
    Framebuffer(const Framebuffer&) = delete; // copying the framebuffer would call the destructor (destroying the actual openGL framebuffer) but create a new framebuffer that thinks that openGL framebuffer still exists (bad)

    ~Framebuffer();

    // Binds the framebuffer, causing all drawing operations to be drawn onto this framebuffer until Bind() is called on another framebuffer, or Unbind() is called.
    void Bind();

    // Unbinds whatever framebuffer is currently bound, so that all drawing operations are drawn on the window.
    static void Unbind();

    const unsigned int width;
    const unsigned int height;

    // these store what is rendered onto the framebuffer
    std::vector<Texture> textureAttachments;

    private:
    GLuint glFramebufferId;
    const GLenum bindingLocation; // TODO, currently always GL_FRAMEBUFFER

    

    // std::optional<unsigned int> colorRenderbufferId; i don't think we actually want this?
    std::optional<unsigned int> depthRenderbufferId;

    friend class Texture;
    
    
};