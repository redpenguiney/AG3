#include "framebuffer.hpp"
#include <cassert>

Framebuffer::Framebuffer(const unsigned int fbWidth, const unsigned int fbHeight, const std::vector<TextureCreateParams>& attachmentParams, const bool haveDepthRenderbuffer): 
width(fbWidth), 
height(fbHeight) 
{
    // create openGL framebuffer
    glGenFramebuffers(1, &glFramebufferId);

    // bind it so we can set it up
    StartDrawingWith();

    // attach textures
    for (auto & params: attachmentParams) {
        textureAttachments.emplace_back(*this, params);
    }

    // create renderbuffer
    if (haveDepthRenderbuffer) {

    }

    // make sure framebuffer is "complete"? 
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
}

Framebuffer::~Framebuffer() {
    glDeleteFramebuffers(1, &glFramebufferId);
}

// TODO: apparently sometimes you want to use an argument besides GL_FRAMEBUFFER?
void Framebuffer::StartDrawingWith() {
    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, glFramebufferId);
}
