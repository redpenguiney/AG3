#include "gengine.hpp"
#include "shader_program.hpp"
#include "debug/assert.hpp"
#include <cstring>
#include <memory>
#include <string>
#include <cstdio>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "GL/glew.h"
#include "glm/glm.hpp"
//#include <optional>

// TODO: custom exceptions

using namespace std::string_literals;

std::string LoadFile(const char* path) {
    std::ifstream stream(path);
    if (stream.fail()) { // Verify file was successfully found/open
        throw std::runtime_error("Unable to find or read the file at path "s + path + " during shader compilation."s);        
    }
    return {std::istreambuf_iterator<char>(stream), {}};
}

std::string Shader::GetInfoLog() {
    int InfoLogLength = 0;
    int CharsWritten = 0;

    glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &InfoLogLength);

    if (InfoLogLength > 0)
    {
        GLchar* InfoLog = new GLchar[InfoLogLength];
        glGetShaderInfoLog(shaderId, InfoLogLength, &CharsWritten, InfoLog);
        return std::string(InfoLog);
        delete[] InfoLog;
    };

    Assert(false);
}

Shader::Shader(const char* path, GLenum shaderType, const std::vector<const char*>& includedFiles) {   
    // Get string from file contents 
    std::string mainShaderSource = LoadFile(path);
    const char* mainShaderSourcePtr = mainShaderSource.c_str();
    GLint mainSourceLength = mainShaderSource.length();

    // tell opengl about the files the shader source wants to include
    Assert(includedFiles.empty()); // openGL support for #include is a sham, we're gonna have to add support ourselves at some point
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
        throw std::runtime_error("Failed to compile \""s + path + "\". GLSL compiler log:\n" + GetInfoLog());
    };
}

Shader::~Shader() {
    glDeleteShader(shaderId);
}

void ShaderProgram::SetCameraUniforms(glm::mat4x4 cameraProjMatrix, glm::mat4x4 cameraProjMatrixNoFloatingOrigin, glm::mat4x4 orthrographicMatrix) {  
    for (auto & [shaderId, shaderProgram] : MeshGlobals::Get().LOADED_PROGRAMS) {
        (void)shaderId;
        if (shaderProgram->useOrthro) { // make sure the shader program actually wants our camera/projection matrix
            shaderProgram->Uniform("orthro", orthrographicMatrix, false);
        }
        if (shaderProgram->usePerspective) {
            shaderProgram->Uniform("perspective", (shaderProgram->useFloatingOrigin) ? cameraProjMatrix : cameraProjMatrixNoFloatingOrigin, false);
        }
    }
}

std::shared_ptr<ShaderProgram> ShaderProgram::New(const char* vertexPath, const char* fragmentPath, const std::vector<const char*>& additionalIncludedFiles, const bool floatingOrigin, const bool usePerspective, const bool useLightClusters, const bool orthrographicProjection, const bool ignorePostProc) {
    auto ptr = std::shared_ptr<ShaderProgram>(new ShaderProgram(vertexPath, fragmentPath, additionalIncludedFiles, floatingOrigin, usePerspective, useLightClusters, orthrographicProjection, ignorePostProc));
    MeshGlobals::Get().LOADED_PROGRAMS.emplace(ptr->programId, ptr);
    return ptr;
}

std::shared_ptr<ShaderProgram>& ShaderProgram::Get(unsigned int shaderProgramId) {
    Assert(MeshGlobals::Get().LOADED_PROGRAMS.count(shaderProgramId) != 0 && "ShaderProgram::Get() was given an invalid shaderProgramId.");
    return MeshGlobals::Get().LOADED_PROGRAMS[shaderProgramId];
}

void ShaderProgram::Unload(unsigned int shaderProgramId) {
    Assert(GraphicsEngine::Get().IsShaderProgramInUse(shaderProgramId));
    Assert(MeshGlobals::Get().LOADED_PROGRAMS.count(shaderProgramId) != 0 && "ShaderProgram::Unload() was given an invalid shaderProgramId.");
    MeshGlobals::Get().LOADED_PROGRAMS.erase(shaderProgramId);
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
    if (MeshGlobals::Get().LOADED_PROGRAM_ID != programId) {
        glUseProgram(programId);
        MeshGlobals::Get().LOADED_PROGRAM_ID = programId;
    }
}

ShaderProgram::ShaderProgram(const char* vertexPath, const char* fragmentPath, const std::vector<const char*>& additionalIncludedFiles, const bool floatingOrigin, const bool perspectiveProjection, const bool useLightClusters, const bool orthrographic, const bool ignorePostProc):
ignorePostProc(ignorePostProc),
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