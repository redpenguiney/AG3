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



void ShaderProgram::SetCameraUniforms(glm::mat4x4 cameraProjMatrix, glm::mat4x4 cameraProjMatrixNoFloatingOrigin, glm::mat4x4 orthrographicMatrix) {  
    for (auto & [shaderId, shaderProgram] : LOADED_SHADER_PROGRAMS) {
        (void)shaderId;
        if (shaderProgram->HasUniform("orthro")) { // make sure the shader program actually wants our camera/projection matrix
            shaderProgram->Uniform("orthro", orthrographicMatrix, false);
        }
        if (shaderProgram->HasUniform("perspective")) {
            shaderProgram->Uniform("perspective", (shaderProgram->useFloatingOrigin) ? cameraProjMatrix : cameraProjMatrixNoFloatingOrigin, false);
        }
    }
}

std::shared_ptr<ShaderProgram> ShaderProgram::New(const char* vertexPath, const char* fragmentPath, const bool floatingOrigin, const bool useLightClusters, const bool ignorePostProc) {
    auto ptr = std::shared_ptr<ShaderProgram>(new ShaderProgram(vertexPath, fragmentPath, floatingOrigin, useLightClusters, ignorePostProc));
    LOADED_PROGRAMS.emplace(ptr->shaderProgramId, ptr);
    LOADED_SHADER_PROGRAMS.emplace(ptr->shaderProgramId, ptr);
    return ptr;
}

std::shared_ptr<ShaderProgram> ShaderProgram::Get(unsigned int shaderProgramId) {
    Assert(LOADED_SHADER_PROGRAMS.count(shaderProgramId) != 0 && "ShaderProgram::Get() was given an invalid shaderProgramId.");
    //auto ptr = std::dynamic_pointer_cast<ShaderProgram>(LOADED_PROGRAMS[shaderProgramId]); // mwahahaha
    //Assert(ptr != nullptr); // make sure this is a pointer to a ShaderProgram, not a ComputeShaderProgram/etc.
    return LOADED_SHADER_PROGRAMS[shaderProgramId];
}



void ShaderProgram::Unload(unsigned int id)
{
    //Assert(!GraphicsEngine::Get().IsShaderProgramInUse(id)); // don't let them unload a shader program if it's being used
    Assert(LOADED_PROGRAMS.count(id) != 0 && "ShaderProgram::Unload() was given an invalid shaderProgramId.");
    LOADED_PROGRAMS.erase(id);
    BaseShaderProgram::Unload(id);
}

ShaderProgram::~ShaderProgram() {
    //glDeleteProgram(programId);
}





ShaderProgram::ShaderProgram(const char* vertexPath, const char* fragmentPath, const bool floatingOrigin, const bool useLightClusters, const bool ignorePostProc):
    BaseShaderProgram(),
    ignorePostProc(ignorePostProc),
    useFloatingOrigin(floatingOrigin),
    useClusteredLighting(useLightClusters),
    vertex(vertexPath, GL_VERTEX_SHADER),
    fragment(fragmentPath, GL_FRAGMENT_SHADER)
{

    // attach shaders to program
    glAttachShader(shaderProgramId, vertex.shaderId);
    glAttachShader(shaderProgramId, fragment.shaderId);

    glBindFragDataLocation(shaderProgramId, 0, "color"); // tell opengl that the variable we're putting the final pixel color in is called "color"

    Link();
}