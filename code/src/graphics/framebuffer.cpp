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

    // attach textures (TODO: support for texture arrays, cubemaps?)
    int attachmentI = 0; // TODO: is there stuff besides color attachments we need to care about? 
    for (auto & params: attachmentParams) {
        //if (attachmentI == 0)
            textureAttachments.emplace_back(*this, params, attachmentI, Texture::Texture2DFlat, GL_COLOR_ATTACHMENT0 + attachmentI);
        //else
            //new Texture(TextureCreateParams({ TextureSource("../textures/grass.png") }, Texture::ColorMap), 0, Texture::Texture2D);
        colorAttachmentNames.emplace_back(GL_COLOR_ATTACHMENT0 + attachmentI);
        attachmentI++;
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
    if (currentlyBound == glFramebufferId) {
        currentlyBound = 0;
    }
    glDeleteFramebuffers(1, &glFramebufferId);
    if (depthRenderbufferId) {
        glDeleteRenderbuffers(1, &depthRenderbufferId.value());
    }
}

// TODO: apparently sometimes you want to use an argument besides GL_FRAMEBUFFER?
void Framebuffer::Bind() {
    if (currentlyBound == glFramebufferId) {
        //DebugLogInfo("Failed to bind");
        return;
        
    }
    else {
        //DebugLogInfo("Bound framebuffer. (prev was ", currentlyBound, " now ", glFramebufferId, ")");
        currentlyBound = glFramebufferId;
        
    }

    glBindFramebuffer(bindingLocation, glFramebufferId);

    //const GLenum buffers[]{ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(colorAttachmentNames.size(), colorAttachmentNames.data());
    glViewport(0, 0, width, height);
}

void Framebuffer::Clear(std::vector<glm::vec4> clearColors)
{
    Assert(clearColors.size() == colorAttachmentNames.size());

    Bind();
    for (int i = 0; i < clearColors.size(); i++) {
        //if (textureAttachments[i].format == Texture::TextureFormat::RGBA_16Float) {
            glClearBufferfv(GL_COLOR, i, &clearColors[i][0]);
        //}
        //else {
            //glm::ivec4 casted =
        //}
    }
}

void Framebuffer::Unbind() {
    currentlyBound = 0;
    unsigned int windowWidth = GraphicsEngine::Get().window.width;
    unsigned int windowHeight = GraphicsEngine::Get().window.height;
    glViewport(0, 0, windowWidth, windowHeight);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // TODO: other binding locations
    glDrawBuffer(GL_FRONT_AND_BACK);
}