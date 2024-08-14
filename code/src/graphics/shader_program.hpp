#pragma once
#include <memory>
#include <string>
#include <fstream>
#include <unordered_map>
#include <vector>
#include "glm/glm.hpp"
#include "base_shader_program.hpp"

// TODO: custom procedural textures


class ShaderProgram;

// Shader program used for rendering, contains (at minimum) a vertex shader and a fragment (basically per pixel) shader.
class ShaderProgram: public BaseShaderProgram {
    public:
    const bool ignorePostProc; // if true, stuff with this shader will be rendered after postprocessing (on top of the screen quad)
    //const bool usePerspective; // if true, the uniform mat4 "perspective" in this program's vertex shader will be automatically set to a perspective projection + view matrix.
    //const bool useOrthro; // if true, the uniform mat4 "orthro" in this program's vertex shader will automatically be set to an orthrographic projection matrix.
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
    static std::shared_ptr<ShaderProgram> New(const char* vertexPath, const char* fragmentPath, const std::vector<const char*>& additionalIncludedFiles = {}, const bool floatingOrigin = true, const bool useLightClusters = true, const bool ignorePostProc = false);

    // Creates a compute shader for performing arbitrary GPU calculations.
    // Returns id of generated program.
    //static std::shared_ptr<ShaderProgram> NewCompute(const char* computePath);

    // Get a pointer to a shader program by its id.
    static std::shared_ptr<ShaderProgram> Get(unsigned int shaderProgramId);

    // unloads the program with the given shaderProgramId, freeing its memory.
    // Calling this function while objects still use the shader will surely crash.
    // All this does is remove a shared_ptr in the LOADED_PROGRAMS map, so if you have other stored references then the ShaderProgram will stubbornly persist.
    // You only need to call this if for whatever reason you are repeatedly swapping out shader programs (like bc ur joining different servers with different resources)
    // TODO very untested
    static void Unload(unsigned int shaderProgramId);

    ~ShaderProgram();

    

    

    private:

    static inline std::unordered_map<unsigned int, std::shared_ptr<ShaderProgram>> LOADED_SHADER_PROGRAMS;
    
    Shader vertex; // processes each vertex 
    Shader fragment; // processes each fragment/pixel
    // TODO: tesselation, geometry shaders

    ShaderProgram(const char* vertexPath, const char* fragmentPath, const std::vector<const char*>& additionalIncludedFiles, const bool floatingOrigin, const bool useLightClusters, const bool ignorePostProc);
    //ShaderProgram(const char* computePath);

};