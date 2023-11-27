#include "../../external_headers/GLEW/glew.h"
#include <optional>
#include "texture.hpp"

class Framebuffer {
    public:


    private:
    std::optional<Texture> colorAttachment;
    
    unsigned int width;
    unsigned int height;
    GLuint glFramebufferId;
};