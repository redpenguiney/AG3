#pragma once

#include <string>
#include <cstdio>
#include <fstream>
#include <unordered_map>
#include "../../external_headers/GLEW/glew.h"
#include "../../external_headers/GLM/glm.hpp"
//#include <optional>

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
    //bool useCameraMatrix; // if true the uniform mat4s "camera" and "proj" in this program's vertex shader will be automatically set

    ShaderProgram(const char* vertexPath, const char* fragmentPath)
    : vertex(vertexPath, GL_VERTEX_SHADER),
    fragment(fragmentPath, GL_FRAGMENT_SHADER) {
        // Create shader program and attach vertex/fragment to it
        programId = glCreateProgram();
        glAttachShader(programId, vertex.shaderId);
        glAttachShader(programId, fragment.shaderId);

        glBindFragDataLocation(programId, 0, "color"); // tell opengl that the variable we're putting the final pixel color in is called "color"
        glLinkProgram(programId);
        int success;
        char infolog[512];
        glGetProgramiv(programId, GL_LINK_STATUS, &success);
        if(!success)
        {
            glGetProgramInfoLog(programId, 512, NULL, infolog);
            printf("\nFailed to link shader program!\n%s", infolog);
            abort();
        }
    }

    ~ShaderProgram() {
        glDeleteProgram(programId);
    }

    // sets the uniform variable in this shader program with the given name to the given value
    void UniformMat4x4(std::string uniformName, glm::mat4x4 matrix, bool transposeMatrix = false) {
        if (uniform_locations.count(uniformName) == 0) {
            uniform_locations[uniformName] = glGetUniformLocation(programId, uniformName.c_str());
        };
        Use();
        glUniformMatrix4fv(uniform_locations.at(uniformName), 1, transposeMatrix, &matrix[0][0]);
    }

    void Use() {
        if (LOADED_PROGRAM_ID != programId) {
            glUseProgram(programId);
            LOADED_PROGRAM_ID = programId;
        }
    }

    private:
    static inline GLuint LOADED_PROGRAM_ID = 0;
    GLuint programId;
    Shader vertex;
    Shader fragment;
    std::unordered_map<std::string, int> uniform_locations; // used to set uniform variables
};