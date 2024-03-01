#include "framebuffer.hpp"
#include <cassert>
#include "engine.hpp"

Framebuffer::Framebuffer(const unsigned int fbWidth, const unsigned int fbHeight, const std::vector<TextureCreateParams>& attachmentParams, const bool haveDepthRenderbuffer): 
width(fbWidth), 
height(fbHeight),
bindingLocation(GL_FRAMEBUFFER)
{
    std::cout << " constructing framebuffer.\n";
    // create openGL framebuffer
    glGenFramebuffers(1, &glFramebufferId);

    // bind it so we can set it up
    Bind();
    std::cout << " bound framebuffer.\n";

    // attach textures
    for (auto & params: attachmentParams) {
        textureAttachments.emplace_back(*this, params, 0, Texture::Texture2D, GL_COLOR_ATTACHMENT0);
    }

    std::cout << " attached textures.\n";
    
    // create renderbuffer if they want it
    if (haveDepthRenderbuffer) {
        std::cout << "have renderbuffer.\n";
        unsigned int rbo;
        glGenRenderbuffers(1, &rbo);
        depthRenderbufferId = rbo; std::cout << "k.\n";
        glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbufferId.value()); std::cout << "so be it.\n";
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height); std::cout << "oh?\n";
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);  
    }

    std::cout << " created renderbuffer.\n";

    // make sure framebuffer is "complete"? 
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    std::cout << " framebuffer is complete.\n";
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