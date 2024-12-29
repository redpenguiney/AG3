#pragma once
#include "GL/glew.h"
#include <optional>
#include <vector>
#include "texture.hpp"
#include "glm/vec4.hpp"

// A framebuffer in OpenGL is an object you draw to.
class Framebuffer {
    public:
    Framebuffer(const unsigned int fbWidth, const unsigned int fbHeight, const std::vector<TextureCreateParams>& attachmentParams, const bool haveDepthRenderbuffer);
    Framebuffer(const Framebuffer&) = delete; // copying the framebuffer would call the destructor (destroying the actual openGL framebuffer) but create a new framebuffer that thinks that openGL framebuffer still exists (bad)

    ~Framebuffer();

    // Binds the framebuffer, causing all drawing operations to be drawn onto this framebuffer until Bind() is called on another framebuffer, or Unbind() is called.
    void Bind();

    // Provide a color for each attachment in textureAttachments.
    // For each attachment, the color channels should be in the correct range (integers in [0, 255] for 8 bit rgb, for example) but this isn't enforced.
    void Clear(std::vector<glm::vec4> clearColors);

    // Unbinds whatever framebuffer is currently bound, so that all drawing operations are drawn on the window.
    static void Unbind();

    const unsigned int width;
    const unsigned int height;

    // these store what is rendered onto the framebuffer
    std::vector<Texture> textureAttachments;

    private:
    // id of currently bound framebuffer, 0 if none (aka the internal default framebuffer) is bound.
    // TOOD: other binding locations
    static inline int currentlyBound = 0; 

    GLuint glFramebufferId;
    const GLenum bindingLocation; // TODO, currently always GL_FRAMEBUFFER

    std::vector<GLenum> colorAttachmentNames = {};

    // std::optional<unsigned int> colorRenderbufferId; i don't think we actually want this?
    std::optional<unsigned int> depthRenderbufferId;

    friend class Texture;
    
    
};