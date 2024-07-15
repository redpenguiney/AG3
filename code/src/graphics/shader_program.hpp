#pragma once
#include <memory>
#include <string>
#include <fstream>
#include <unordered_map>
#include <vector>
#include "GL/glew.h"
#include "glm/glm.hpp"

// TODO: custom procedural textures

// In OpenGL, a shader program contains (at minimum) a vertex shader and a fragment (basically per pixel) shader.
class ShaderProgram;
class Shader {
    friend class ShaderProgram;
    Shader(const char* path, GLenum shaderType, const std::vector<const char*>& includedFiles);
    std::string GetInfoLog();
    ~Shader();

    GLuint shaderId;
};

class ShaderProgram {
    public:
    // lets programId be read only without a getter
    const unsigned int& shaderProgramId = programId;

    const bool usePerspective; // if true, the uniform mat4 "perspective" in this program's vertex shader will be automatically set to a perspective projection + view matrix.
    const bool useOrthro; // if true, the uniform mat4 "orthro" in this program's vertex shader will automatically be set to an orthrographic projection matrix.
    const bool useFloatingOrigin; // if true, camera translation will be done in the shader instead of with doubles on the gpu
    const bool useClusteredLighting; // if true, the ssbo "clusters" in this program's vertex & fragment shaders will be automatically bound
    // const std::optional<Framebuffer> RenderTo; TODO: for stuff like viewport frames. things rendered with this shader will be rendered onto the framebuffer contained in the optional, or onto the main framebuffer if it's nullopt.

    // TODO: the graphics engine should really just do this itself. 
    // Passes projection/camera matrix to shaders that have useCameraMatrix == true.
    // Takes two different matrices, for shaders that don't use floating origin
    // Called by GraphicsEngine.
    static void SetCameraUniforms(glm::mat4x4 cameraProjMatrix, glm::mat4x4 cameraProjMatrixNoFloatingOrigin, glm::mat4x4 orthrographicMatrix);

    // Returns id of generated program.
    // additionalIncludedFiles is an optional vector of filepaths to files included by the other shaders.
    static std::shared_ptr<ShaderProgram> New(const char* vertexPath, const char* fragmentPath, const std::vector<const char*>& additionalIncludedFiles = {}, const bool floatingOrigin = true, const bool perspectiveProjection = true, const bool useLightClusters = true, const bool orthrographicProjection = false);

    // Creates a compute shader for performing arbitrary GPU calculations.
    // Returns id of generated program.
    static std::shared_ptr<ShaderProgram> NewCompute(const char* computePath);

    // Get a pointer to a shader program by its id.
    static std::shared_ptr<ShaderProgram>& Get(unsigned int shaderProgramId);

    // unloads the program with the given shaderProgramId, freeing its memory.
    // Calling this function while objects still use the shader will error.
    // You only need to call this if for whatever reason you are repeatedly swapping out shader programs (like bc ur joining different servers with different resources)
    static void Unload(unsigned int shaderProgramId);

    ~ShaderProgram();

    // sets the uniform variable in this shader program with the given name to the given value
    void Uniform(std::string uniformName, glm::mat4x4 matrix, bool transposeMatrix = false);
    void Uniform(std::string uniformName, glm::vec4 vec);
    void Uniform(std::string uniformName, glm::vec3 vec);
    void Uniform(std::string uniformName, float fval);
    void Uniform(std::string uniformName, bool bval);

    void Use();

    private:
    
    GLuint programId;
    Shader vertex; // processes each vertex 
    Shader fragment; // processes each fragment/pixel
    // TODO: tesselation, geometry, maybe even compute shaders
    std::unordered_map<std::string, int> uniform_locations; // used to set uniform variables
    

    ShaderProgram(const char* vertexPath, const char* fragmentPath, const std::vector<const char*>& additionalIncludedFiles, const bool floatingOrigin, const bool usePerspective, const bool useLightClusters, const bool orthrographic);
    ShaderProgram(const char* computePath);

};