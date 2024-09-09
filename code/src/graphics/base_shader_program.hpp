#pragma once
#include "GL/glew.h"
#include <unordered_map>
#include <memory>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

class Shader {
    friend class BaseShaderProgram;
    friend class ShaderProgram;
    friend class ComputeShaderProgram;
    Shader(const char* path, GLenum shaderType, const std::vector<const char*>& includedFiles);
    std::string GetInfoLog();
    ~Shader();

    GLuint shaderId;
};


// Base class that both the normal ShaderProgram (for rendering) and ComputeShaderProgram (for gpu computation) classes derive from.
class BaseShaderProgram {
    public:
    // lets programId be read only without a getter
    const unsigned int& shaderProgramId = programId;

    // returns true if the shader uses the given uniform
    bool HasUniform(std::string name);

    // sets the uniform variable in this shader program with the given name to the given value
    void Uniform(std::string uniformName, glm::mat4x4 matrix, bool transposeMatrix = false);
    void Uniform(std::string uniformName, glm::vec4 vec);
    void Uniform(std::string uniformName, glm::vec3 vec);
    void Uniform(std::string uniformName, float fval);
    void Uniform(std::string uniformName, bool bval);
    void Uniform(std::string uniformName, unsigned int uval);

    virtual ~BaseShaderProgram();

    // Binds the shader program so things are drawn with it (in the case of ShaderProgram)
    void Use();

protected:
    BaseShaderProgram();

    // only call once, obviously (after shaders are attached)
    void Link();

    // protected so factory constructors can add it
    static inline std::unordered_map<unsigned int, std::shared_ptr<BaseShaderProgram>> LOADED_PROGRAMS;

    // unloads the program with the given shaderProgramId, freeing its memory.
    // Calling this function while objects still use the shader is fine, meshpool stores another shared_ptr to the shaders it uses.
    // All this does is remove a shared_ptr in the LOADED_PROGRAMS map, so if you have other stored references then the ShaderProgram will stubbornly persist.
    // You only need to call this if for whatever reason you are repeatedly swapping out shader programs (like bc ur joining different servers with different resources)
    // TODO very untested
    static void Unload(unsigned int shaderProgramId);

private:

    

    static inline GLuint LOADED_PROGRAM_ID = 0; // id of the currently loaded shader program
    

    std::unordered_map<std::string, int> uniform_locations; // used to set uniform variables

    GLuint programId;
};