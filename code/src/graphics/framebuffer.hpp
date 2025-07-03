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
    // The nth fragment shader output will write to the nth framebuffer attachment.
    void Bind();

    // Binds the framebuffer, causing all drawing operations to be drawn onto this framebuffer until Bind() is called on another framebuffer, or Unbind() is called.
    // The nth fragment shader output will write to the attachmentIndices[n]th framebuffer attachment. Not all attachments need to be used.
    // Do not pass duplicate attachments.
    void Bind(std::vector<size_t> attachmentIndices);

    // Provide a color for each attachment that is currently being drawn to, in the order implied by the attachmentIndices called to Bind() if provided, or otherwise for every texture attachment.
    // For each attachment, the color channels should be in the correct range (integers in [0, 255] for 8 bit rgb, for example) but this isn't enforced.
        // Pass vec4(-1, -1, -1, -1) to not clear that attachment.
    // Framebuffer MUST already be bound or this will error.
    void Clear(std::vector<glm::vec4> clearColors);

    // Clears the depth renderbuffer (erroring if it doesn't exist)
    // Framebuffer MUST already be bound or this will error.
    void ClearDepthRenderbuffer();

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

    // indices of the textureAttachments that are currently being used as a draw buffer (being drawn to)
    static inline std::vector<size_t> currentDrawBuffers = {};

    GLuint glFramebufferId;
    const GLenum bindingLocation; // TODO, currently always GL_FRAMEBUFFER

    // the attachment (e.g GL_COLOR_ATTACHMENT0) corresponding to each element of textureAttachments
    std::vector<GLenum> colorAttachmentNames = {};

    // std::optional<unsigned int> colorRenderbufferId; i don't think we actually want this?
    std::optional<unsigned int> depthRenderbufferId;

    friend class Texture;
    
    
};