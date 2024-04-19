#include "engine.hpp"
#include "shader_program.hpp"
#include <cassert>
#include <cstring>
#include <memory>
#include <string>
#include <cstdio>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "../../external_headers/GLEW/glew.h"
#include "../../external_headers/GLM/glm.hpp"
//#include <optional>

std::string LoadFile(const char* path) {
    std::ifstream stream(path);
    if (stream.fail()) { // Verify file was successfully found/open
        printf("\nUnable to find or read the file at path %s during shader compilation. Aborting.", path);
        abort();
    }
    return {std::istreambuf_iterator<char>(stream), {}};
}

Shader::Shader(const char* path, GLenum shaderType, const std::vector<const char*>& includedFiles) {   
    // Get string from file contents 
    std::string mainShaderSource = LoadFile(path);
    const char* mainShaderSourcePtr = mainShaderSource.c_str();
    GLint mainSourceLength = mainShaderSource.length();

    // tell opengl about the files the shader source wants to include
    assert(includedFiles.empty()); // openGL support for #include is a sham, we're gonna have to add support ourselves at some point
    // for (auto & includePath: includedFiles) {
    //     std::string source = LoadFile(includePath);
    //     const char* sourcePtr = source.c_str();
    //     GLint sourceLength = source.length();
    //     glNamedStringARB(GL_SHADER_INCLUDE_ARB, strlen(includePath), includePath, sourceLength, sourcePtr);
    // }

    // Compile the shader
    GLint compiled;
    shaderId = glCreateShader(shaderType);
    glShaderSource(shaderId, 1, &mainShaderSourcePtr, &mainSourceLength);
    glCompileShader(shaderId);
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, & compiled);
    if (compiled != GL_TRUE){
        printf("\nFailed to compile %s. Aborting.", path);
        PrintInfoLog();
        abort();
    };
}

Shader::~Shader() {
    glDeleteShader(shaderId);
}

void ShaderProgram::SetCameraUniforms(glm::mat4x4 cameraProjMatrix, glm::mat4x4 cameraProjMatrixNoFloatingOrigin, glm::mat4x4 orthrographicMatrix) {  
    for (auto & [shaderId, shaderProgram] : LOADED_PROGRAMS) {
        (void)shaderId;
        if (shaderProgram->useOrthro) { // make sure the shader program actually wants our camera/projection matrix
            shaderProgram->Uniform("orthro", orthrographicMatrix, false);
        }
        if (shaderProgram->usePerspective) {
            shaderProgram->Uniform("perspective", (shaderProgram->useFloatingOrigin) ? cameraProjMatrix : cameraProjMatrixNoFloatingOrigin, false);
        }
    }
}

std::shared_ptr<ShaderProgram> ShaderProgram::New(const char* vertexPath, const char* fragmentPath, const std::vector<const char*>& additionalIncludedFiles, const bool floatingOrigin, const bool useCameraUniform, const bool useLightClusters, const bool orthrographicProjection) {
    auto ptr = std::shared_ptr<ShaderProgram>(new ShaderProgram(vertexPath, fragmentPath, additionalIncludedFiles, floatingOrigin, useCameraUniform, useLightClusters, orthrographicProjection));
    LOADED_PROGRAMS.emplace(ptr->programId, ptr);
    return ptr;
}

std::shared_ptr<ShaderProgram>& ShaderProgram::Get(unsigned int shaderProgramId) {
    assert(LOADED_PROGRAMS.count(shaderProgramId) != 0 && "ShaderProgram::Get() was given an invalid shaderProgramId.");
    return LOADED_PROGRAMS[shaderProgramId];
}

void ShaderProgram::Unload(unsigned int shaderProgramId) {
    assert(GraphicsEngine::Get().IsShaderProgramInUse(shaderProgramId));
    assert(LOADED_PROGRAMS.count(shaderProgramId) != 0 && "ShaderProgram::Unload() was given an invalid shaderProgramId.");
    LOADED_PROGRAMS.erase(shaderProgramId);
}

ShaderProgram::~ShaderProgram() {
    glDeleteProgram(programId);
}

void ShaderProgram::Uniform(std::string uniformName, glm::mat4x4 matrix, bool transposeMatrix) {
    if (uniform_locations.count(uniformName) == 0) {
        uniform_locations[uniformName] = glGetUniformLocation(programId, uniformName.c_str());
    };
    Use();
    glUniformMatrix4fv(uniform_locations.at(uniformName), 1, transposeMatrix, &matrix[0][0]);
}

void ShaderProgram::Uniform(std::string uniformName, glm::vec4 vec) {
    if (uniform_locations.count(uniformName) == 0) {
        uniform_locations[uniformName] = glGetUniformLocation(programId, uniformName.c_str());
    };
    Use();
    glUniform4fv(uniform_locations.at(uniformName), 1, &vec.x);
}

void ShaderProgram::Uniform(std::string uniformName, glm::vec3 vec) {
    if (uniform_locations.count(uniformName) == 0) {
        uniform_locations[uniformName] = glGetUniformLocation(programId, uniformName.c_str());
    };
    Use();
    glUniform3fv(uniform_locations.at(uniformName), 1, &vec.x);
}

void ShaderProgram::Uniform(std::string uniformName, float fval) {
    if (uniform_locations.count(uniformName) == 0) {
        uniform_locations[uniformName] = glGetUniformLocation(programId, uniformName.c_str());
    };
    Use();
    glUniform1f(uniform_locations.at(uniformName), fval);
}

void ShaderProgram::Uniform(std::string uniformName, bool bval) {
    if (uniform_locations.count(uniformName) == 0) {
        uniform_locations[uniformName] = glGetUniformLocation(programId, uniformName.c_str());
    };
    Use();
    glUniform1i(uniform_locations.at(uniformName), bval);
}

void ShaderProgram::Use() {
    if (LOADED_PROGRAM_ID != programId) {
        glUseProgram(programId);
        LOADED_PROGRAM_ID = programId;
    }
}

ShaderProgram::ShaderProgram(const char* vertexPath, const char* fragmentPath, const std::vector<const char*>& additionalIncludedFiles, const bool floatingOrigin, const bool perspectiveProjection, const bool useLightClusters, const bool orthrographic): 
usePerspective(perspectiveProjection),
useOrthro(orthrographic),
useFloatingOrigin(floatingOrigin),
useClusteredLighting(useLightClusters),
vertex(vertexPath, GL_VERTEX_SHADER, additionalIncludedFiles),
fragment(fragmentPath, GL_FRAGMENT_SHADER, additionalIncludedFiles)
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