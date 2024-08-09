#include "framebuffer.hpp"
#include "debug/assert.hpp"
#include "gengine.hpp"

Framebuffer::Framebuffer(const unsigned int fbWidth, const unsigned int fbHeight, const std::vector<TextureCreateParams>& attachmentParams, const bool haveDepthRenderbuffer): 
width(fbWidth), 
height(fbHeight),
bindingLocation(GL_FRAMEBUFFER)
{
    // create openGL framebuffer
    glGenFramebuffers(1, &glFramebufferId);

    // bind it so we can set it up
    Bind();

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
    Assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
}

Framebuffer::~Framebuffer() {
    glDeleteFramebuffers(1, &glFramebufferId);
    if (depthRenderbufferId) {
        glDeleteRenderbuffers(1, &depthRenderbufferId.value());
    }
}

// TODO: apparently sometimes you want to use an argument besides GL_FRAMEBUFFER?
void Framebuffer::Bind() {
    glViewport(0, 0, width, height);
    glBindFramebuffer(bindingLocation, glFramebufferId);
}

void Framebuffer::Unbind() {
    unsigned int windowWidth = GraphicsEngine::Get().window.width;
    unsigned int windowHeight = GraphicsEngine::Get().window.height;
    glViewport(0, 0, windowWidth, windowHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // TODO: other binding locations
}