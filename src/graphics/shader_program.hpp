#pragma once
#include <memory>
#include <string>
#include <fstream>
#include <unordered_map>
#include <vector>
#include "../../external_headers/GLEW/glew.h"
#include "../../external_headers/GLM/glm.hpp"

// TODO: custom procedural textures

// In OpenGL, a shader program contains (at minimum) a vertex shader and a fragment (basically per pixel) shader.
class ShaderProgram;
class Shader {
    friend class ShaderProgram;
    Shader(const char* path, GLenum shaderType, const std::vector<const char*>& includedFiles);

    ~Shader();

    void PrintInfoLog() {
        int InfoLogLength = 0;
        int CharsWritten = 0;

        glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, & InfoLogLength);

        if (InfoLogLength > 0)
        {
            GLchar * InfoLog = new GLchar[InfoLogLength];
            glGetShaderInfoLog(shaderId, InfoLogLength, & CharsWritten, InfoLog);
            std::printf("\nShader Info Log: %s", InfoLog);
            delete [] InfoLog;
        };
    }

    GLuint shaderId;
};

class ShaderProgram {
    public:
    // lets programId be read only without a getter
    const unsigned int& shaderProgramId = programId;

    const bool useCameraMatrix; // if true, the uniform mat4 "camera" in this program's vertex shader will be automatically set to a projection + view matrix.
    const bool useFloatingOrigin; // if true, camera translation will be done in the shader instead of with doubles on the gpu
    const bool useClusteredLighting; // if true, the ssbo "clusters" in this program's vertex & fragment shaders will be automatically bound

    // TODO: the graphics engine should really just do this itself. 
    // Passes projection/camera matrix to shaders that have useCameraMatrix == true.
    // Takes two different matrices, for shaders that don't use floating origin
    // Called by GraphicsEngine.
    static void SetCameraUniforms(glm::mat4x4 cameraProjMatrix, glm::mat4x4 cameraProjMatrixNoFloatingOrigin);

    // Returns id of generated program.
    // additionalIncludedFiles is an optional vector of filepaths to files included by the other shaders.
    static std::shared_ptr<ShaderProgram> New(const char* vertexPath, const char* fragmentPath, const std::vector<const char*>& additionalIncludedFiles = {}, const bool floatingOrigin = true, const bool useCameraUniform = true, const bool useLightClusters = true);

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
    static inline GLuint LOADED_PROGRAM_ID = 0;
    GLuint programId;
    Shader vertex; // processes each vertex 
    Shader fragment; // processes each fragment/pixel
    // TODO: tesselation, geometry, maybe even compute shaders
    std::unordered_map<std::string, int> uniform_locations; // used to set uniform variables
    inline static std::unordered_map<unsigned int, std::shared_ptr<ShaderProgram>> LOADED_PROGRAMS; 

    ShaderProgram(const char* vertexPath, const char* fragmentPath, const std::vector<const char*>& additionalIncludedFiles, const bool floatingOrigin, const bool useCameraUniform, const bool useLightClusters);
    ShaderProgram(const char* computePath);

};