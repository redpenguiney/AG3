#pragma once
#include <memory>
#include <string>
#include <fstream>
#include <unordered_map>
#include "../../external_headers/GLEW/glew.h"
#include "../../external_headers/GLM/glm.hpp"

// TODO: custom procedural textures

// In OpenGL, a shader program contains (at minimum) a vertex shader and a fragment (basically per pixel) shader.
class ShaderProgram;
class Shader {
    friend class ShaderProgram;
    Shader(const char* path, GLenum shaderType) {
        
        // Get string from file contents 
        std::ifstream stream(path);
        if (stream.fail()) { // Verify file was successfully found/open
            printf("\nUnable to find or read the file at path %s. Aborting.", path);
            abort();
        }
        std::string shaderSource = {std::istreambuf_iterator<char>(stream), {}};
        const char* shaderSourcePtr = shaderSource.c_str();
        GLint sourceLength = shaderSource.length();

        // Compile the shader
        GLint compiled;
        shaderId = glCreateShader(shaderType);
        glShaderSource(shaderId, 1, &shaderSourcePtr, &sourceLength);
        glCompileShader(shaderId);
        glGetShaderiv(shaderId, GL_COMPILE_STATUS, & compiled);
        if (compiled != GL_TRUE){
            printf("\nFailed to compile %s. Aborting.", path);
            PrintInfoLog();
            abort();
        };
    }

    ~Shader() {
        glDeleteShader(shaderId);
    }

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

    static void SetCameraUniforms(glm::mat4x4 cameraProjMatrix);

    static std::shared_ptr<ShaderProgram> New(const char* vertexPath, const char* fragmentPath, const bool useCameraUniform = true);

    static std::shared_ptr<ShaderProgram>& Get(unsigned int shaderProgramId);

    static void Unload(unsigned int shaderProgramId);

    ~ShaderProgram();

    void UniformMat4x4(std::string uniformName, glm::mat4x4 matrix, bool transposeMatrix = false);

    void Use();

    private:
    static inline GLuint LOADED_PROGRAM_ID = 0;
    GLuint programId;
    Shader vertex; // processes each vertex 
    Shader fragment; // processes each fragment/pixel
    // TODO: tesselation, geometry, maybe even compute shaders
    std::unordered_map<std::string, int> uniform_locations; // used to set uniform variables
    inline static std::unordered_map<unsigned int, std::shared_ptr<ShaderProgram>> LOADED_PROGRAMS; 

    ShaderProgram(const char* vertexPath, const char* fragmentPath, const bool useCameraUniform);

};