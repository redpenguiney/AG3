#include "engine.hpp"
#include "shader_program.hpp"
#include <cassert>
#include <memory>
#include <string>
#include <cstdio>
#include <unordered_map>
#include "../../external_headers/GLEW/glew.h"
#include "../../external_headers/GLM/glm.hpp"
//#include <optional>
 
// Passes projection/camera matrix to shaders that have useCameraMatrix == true.
// Called by GraphicsEngine.
void ShaderProgram::SetCameraUniforms(glm::mat4x4 cameraProjMatrix) {  
    for (auto & [shaderId, shaderProgram] : LOADED_PROGRAMS) {
        (void)shaderId;
        if (shaderProgram->useCameraMatrix) { // make sure the shader program actually wants our camera/projection matrix
            shaderProgram->UniformMat4x4("camera", cameraProjMatrix, false);
        }  
    }
}

// Returns id of generated program
std::shared_ptr<ShaderProgram> ShaderProgram::New(const char* vertexPath, const char* fragmentPath, const bool useCameraUniform) {
    auto ptr = std::shared_ptr<ShaderProgram>(new ShaderProgram(vertexPath, fragmentPath, useCameraUniform));
    LOADED_PROGRAMS.emplace(ptr->programId, ptr);
    return ptr;
}

// Get a pointer to a shader program by its id.
std::shared_ptr<ShaderProgram>& ShaderProgram::Get(unsigned int shaderProgramId) {
    assert(LOADED_PROGRAMS.count(shaderProgramId) != 0 && "ShaderProgram::Get() was given an invalid shaderProgramId.");
    return LOADED_PROGRAMS[shaderProgramId];
}

// unloads the program with the given shaderProgramId, freeing its memory.
// Calling this function while objects still use the shader will error.
// You only need to call this if for whatever reason you are repeatedly swapping out shader programs (like bc ur joining different servers with different resources)
void ShaderProgram::Unload(unsigned int shaderProgramId) {
    assert(GraphicsEngine::Get().IsShaderProgramInUse(shaderProgramId));
    assert(LOADED_PROGRAMS.count(shaderProgramId) != 0 && "ShaderProgram::Unload() was given an invalid shaderProgramId.");
    LOADED_PROGRAMS.erase(shaderProgramId);
}

ShaderProgram::~ShaderProgram() {
    glDeleteProgram(programId);
}

// sets the uniform variable in this shader program with the given name to the given value
void ShaderProgram::UniformMat4x4(std::string uniformName, glm::mat4x4 matrix, bool transposeMatrix) {
    if (uniform_locations.count(uniformName) == 0) {
        uniform_locations[uniformName] = glGetUniformLocation(programId, uniformName.c_str());
    };
    Use();
    glUniformMatrix4fv(uniform_locations.at(uniformName), 1, transposeMatrix, &matrix[0][0]);
}

void ShaderProgram::Use() {
    if (LOADED_PROGRAM_ID != programId) {
        glUseProgram(programId);
        LOADED_PROGRAM_ID = programId;
    }
}

ShaderProgram::ShaderProgram(const char* vertexPath, const char* fragmentPath, const bool useCameraUniform)
: useCameraMatrix(useCameraUniform),
vertex(vertexPath, GL_VERTEX_SHADER),
fragment(fragmentPath, GL_FRAGMENT_SHADER)
    {
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
        printf("\nFailed to link shader program!\n%s\n", infolog);
        abort();
    }
    }