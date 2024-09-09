#include <string>
#include "base_shader_program.hpp"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <debug/assert.hpp>
#include <vector>
#include "mesh.hpp"
#include "graphics/gengine.hpp"

using namespace std::string_literals;

std::string LoadFile(const char* path) {
    std::ifstream stream(path);
    if (stream.fail()) { // Verify file was successfully found/open
        throw std::runtime_error("Unable to find or read the file at path "s + path + " during shader compilation."s);
    }
    return { std::istreambuf_iterator<char>(stream), {} };
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
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        std::string compilationFailure = "Failed to compile \""s + path + "\". GLSL compiler log:\n" + GetInfoLog();
        DebugLogError(compilationFailure);
        throw std::runtime_error(compilationFailure);
    };
}

Shader::~Shader() {
    glDeleteShader(shaderId);
}

void BaseShaderProgram::Unload(unsigned int id) {
    // meshpools hold onto a shared_ptr to the shader so its fine
    //Assert(!GraphicsEngine::Get().IsShaderProgramInUse(id)); // don't let them unload a shader program if it's being used
    Assert(LOADED_PROGRAMS.count(id) != 0 && "BaseShaderProgram::Unload() was given an invalid shaderProgramId.");
    LOADED_PROGRAMS.erase(id);
}

BaseShaderProgram::~BaseShaderProgram()
{
    glDeleteProgram(programId);
}

BaseShaderProgram::BaseShaderProgram()
{
    // Create shader program and attach vertex/fragment to it
    programId = glCreateProgram();
}

bool BaseShaderProgram::HasUniform(std::string name)
{
    if (uniform_locations.count(name) == 0) {
        uniform_locations[name] = glGetUniformLocation(programId, name.c_str());
        //Assert(uniform_locations[uniformName] != -1); // verify that the uniform name exists
    };
    return uniform_locations[name] != -1;
}

void BaseShaderProgram::Link()
{
    glLinkProgram(shaderProgramId);
    int success;
    char infolog[512];
    glGetProgramiv(shaderProgramId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgramId, 512, NULL, infolog);
        printf("\nFailed to link shader program!\n%s\n", infolog);
        abort();
    }
}

void BaseShaderProgram::Uniform(std::string uniformName, glm::mat4x4 matrix, bool transposeMatrix) {
    if (uniform_locations.count(uniformName) == 0) {
        uniform_locations[uniformName] = glGetUniformLocation(programId, uniformName.c_str());
        //Assert(uniform_locations[uniformName] != -1); // verify that the uniform name exists
    };
    Use();
    glUniformMatrix4fv(uniform_locations.at(uniformName), 1, transposeMatrix, &matrix[0][0]);
}

void BaseShaderProgram::Uniform(std::string uniformName, glm::vec4 vec) {
    if (uniform_locations.count(uniformName) == 0) {
        uniform_locations[uniformName] = glGetUniformLocation(programId, uniformName.c_str());
        //Assert(uniform_locations[uniformName] != -1); // verify that the uniform name exists
    };
    Use();
    glUniform4fv(uniform_locations.at(uniformName), 1, &vec.x);
}

void BaseShaderProgram::Uniform(std::string uniformName, glm::vec3 vec) {
    if (uniform_locations.count(uniformName) == 0) {
        uniform_locations[uniformName] = glGetUniformLocation(programId, uniformName.c_str());
        //Assert(uniform_locations[uniformName] != -1); // verify that the uniform name exists
    };
    Use();
    glUniform3fv(uniform_locations.at(uniformName), 1, &vec.x);
}

void BaseShaderProgram::Uniform(std::string uniformName, float fval) {
    if (uniform_locations.count(uniformName) == 0) {
        uniform_locations[uniformName] = glGetUniformLocation(programId, uniformName.c_str());
        //Assert(uniform_locations[uniformName] != -1); // verify that the uniform name exists
    };
    Use();
    glUniform1f(uniform_locations.at(uniformName), fval);
}

void BaseShaderProgram::Uniform(std::string uniformName, bool bval) {
    if (uniform_locations.count(uniformName) == 0) {
        uniform_locations[uniformName] = glGetUniformLocation(programId, uniformName.c_str());
        //Assert(uniform_locations[uniformName] != -1); // verify that the uniform name exists
    };
    Use();
    glUniform1i(uniform_locations.at(uniformName), bval);
}

void BaseShaderProgram::Uniform(std::string uniformName, unsigned int uval)
{
    if (uniform_locations.count(uniformName) == 0) {
        uniform_locations[uniformName] = glGetUniformLocation(programId, uniformName.c_str());
        //Assert(uniform_locations[uniformName] != -1); // verify that the uniform name exists
    };
    Use();
    glUniform1ui(uniform_locations.at(uniformName), uval);
}

void BaseShaderProgram::Use() {
    if (LOADED_PROGRAM_ID != programId) {
        glUseProgram(programId);
        LOADED_PROGRAM_ID = programId;
    }
}
