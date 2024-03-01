#include "framebuffer.hpp"
#include <cassert>

Framebuffer::Framebuffer(const unsigned int fbWidth, const unsigned int fbHeight, const std::vector<TextureCreateParams>& attachmentParams, const bool haveDepthRenderbuffer): 
width(fbWidth), 
height(fbHeight),
bindingLocation(GL_FRAMEBUFFER)
{
    // create openGL framebuffer
    glGenFramebuffers(1, &glFramebufferId);

    // bind it so we can set it up
    StartDrawingWith();

    // attach textures
    for (auto & params: attachmentParams) {
        textureAttachments.emplace_back(*this, params, 0, Texture::Texture2D, GL_COLOR_ATTACHMENT0);
    }

    // create renderbuffer if they want it
    if (haveDepthRenderbuffer) {
        unsigned int rbo;
        glGenRenderbuffers(1, &rbo);
        depthRenderbufferId = rbo;
        glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbufferId.value());
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);  
    }

    // make sure framebuffer is "complete"? 
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
}

Framebuffer::~Framebuffer() {
    glDeleteFramebuffers(1, &glFramebufferId);
}

// TODO: apparently sometimes you want to use an argument besides GL_FRAMEBUFFER?
void Framebuffer::StartDrawingWith() {
    glViewport()
    glBindFramebuffer(GL_FRAMEBUFFER, glFramebufferId);
}

// TODO: again, apparently sometimes you want to use an argument besides GL_FRAMEBUFFER?
void Framebuffer::StopDrawingWith() {
    glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
