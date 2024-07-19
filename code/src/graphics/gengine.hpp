#pragma once
#include "mesh.hpp"
#include "meshpool.hpp"
#include "modules/graphics_engine_export.hpp"
#include "window.hpp"
#include "shader_program.hpp"
#include <cassert>
#include <cstdio>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "../debug/debug.hpp"
#include "camera.hpp"
#include "../gameobjects/transform_component.hpp"
#include "../utility/utility.hpp"
#include "material.hpp"
#include "renderable_mesh.hpp"
#include "framebuffer.hpp"
// #include "gameobjects/render_component.hpp"

// struct MeshLocation {
//     unsigned int poolId; // uuid of the meshpool 
//     unsigned int poolSlot;
//     unsigned int poolInstance;
//     unsigned int materialId;
//     unsigned int shaderProgramId;
//     bool initialized;
//     MeshLocation() {
//         initialized = false;
//     }
// };

class RenderComponent;
class RenderComponentNoFO;

// Handles graphics, obviously.
class GraphicsEngine: public ModuleGraphicsEngineInterface {
    public:
    // the shader used to render the skybox. Can freely change this with no issues.
    std::shared_ptr<ShaderProgram> skyboxShaderProgram;

    // skybox material. Must be a cubemap.
    std::shared_ptr<Material> skyboxMaterial;

    // Which layer of the skyboxMaterial we should actually be using.
    unsigned int skyboxMaterialLayer; 

    // Postprocessing shader, aka the shader used to render the screen quad that the actual world was rendered onto the texture of.
    // Can freely change this with no issues.
    std::shared_ptr<ShaderProgram> postProcessingShaderProgram; 

    // Default shader program used for new RenderComponents. You can freely change this with no issues (I think??? TODO what happens).
    std::shared_ptr<ShaderProgram> defaultShaderProgram;

    // Default shader program used when creating instances of the Gui class. Should be able to freely change this with no issues (TODO verify)
    std::shared_ptr<ShaderProgram> defaultGuiShaderProgram;

    // Same as defaultGuiShaderProgram, but for billboard uis. Should be able to freely change this with no issues (TODO verify)
    std::shared_ptr<ShaderProgram> defaultBillboardGuiShaderProgram;

    // draw wireframe instead of triangles for debugging
    void SetWireframeEnabled(bool);

    // freecam is just a thing for debugging
    bool debugFreecamEnabled = false;
    

    // Returns main camera if debug freecam is disabled, otherwise returns debug freecam camera.
    Camera& GetCurrentCamera();
    Camera& GetMainCamera();
    Camera& GetDebugFreecamCamera();

    void SetDebugFreecamEnabled(bool);
    void SetDebugFreecamPitch(double);
    void SetDebugFreecamYaw(double);
    void SetDebugFreecamAcceleration(double);

    double debugFreecamPitch = 0; // in degrees, don't get tripped up when you do lookvector which wants radians
    double debugFreecamYaw = 0;

    void SetSkyboxShaderProgram(std::shared_ptr<ShaderProgram>);
    void SetSkyboxMaterial(std::shared_ptr<Material>);

    void SetPostProcessingShaderProgram(std::shared_ptr<ShaderProgram>);
    
    void SetDefaultShaderProgram(std::shared_ptr<ShaderProgram>);
    void SetDefaultGuiShaderProgram(std::shared_ptr<ShaderProgram>);
    void SetDefaultBillboardGuiShaderProgram(std::shared_ptr<ShaderProgram>);

    Window& GetWindow();

    // Be advised: If you're tryna get camera position/orientation regardless of whether debugfreecam is enabled, use the GetCurrentCamera() method instead.
    Camera camera;
    
    Window window = Window(720, 720); // handles windowing, interfaces with GLFW in general

    static GraphicsEngine& Get();

    // Used by Texture to throw an error if someone tries to unload a texture being used
    bool IsTextureInUse(unsigned int textureId);

    // Used by ShaderProgram to throw an error if someone tries to unload a shader being used.
    bool IsShaderProgramInUse(unsigned int shaderId);

    // returns true if the user is trying to close the application, or if glfwSetWindowShouldClose was explicitly called (like by a quit game button)
    bool ShouldClose();

    // Draws everything. dt should be time since this function was last called.
    void RenderScene(float dt);

    // Used by mesh loaders when they fail to load a material from a file. (Not used by material/texture constructors, those throw an exception if they fail to load a file.)
    // You can probably change this to whatever you want. (TODO confirm)
    std::shared_ptr<Material> errorMaterial; 
    float errorMaterialTextureZ; 
    // std::shared_ptr<Material> GetDefaultMaterial(); // TODO

    // Render components that use a dynamic mesh add themselves to this unordered map (and remove themselves on destruction).
    // Key is mesh id, value is ptr to rendercomponent which uses that mesh.
    // Needed so that mesh.cpp can access the meshpools being used by objects with dynamic meshes, in order to apply changes made to those dynamic meshes to the GPU.
    std::unordered_map<unsigned int, std::vector<RenderComponent*>> dynamicMeshUsers;

    // used by various debug features, can probably change freely with no isses (TODO check) 
    std::shared_ptr<ShaderProgram> crummyDebugShader;

    // float is dt
    Event<float> preRenderEvent;
    // float is dt; note, fires before buffers are flipped which might be bad idk lol
    Event<float> postRenderEvent;

    private:

    bool wireframeDrawing = false;

    Camera debugFreecamCamera;
    double debugFreecamSpeed = 0;
    double debugFreecamAcceleration = 0.01;

    friend class Mesh; // literally just friend so dynamic mesh support is less work for me idc about keeping it modular
    friend class RenderComponent;
    // friend class RenderComponentNoFO;

    // SSBO that stores all points lights so that the GPU can use them.
    // Format is:
    // struct lightInfo {
        // vec4 colorAndRange; // w-coord is range, xyz is rgb
        // vec4 rel_pos; // w-coord is padding
    // }
    // unsigned int numLights;
    // vec3 padding
    // lightInfo pointLights[];

    
    
    BufferedBuffer pointLightDataBuffer;

    RenderableMesh* skybox; 

    // everything is drawn onto this framebuffer, and then this framebuffer's texture is used to draw a screen quad on the default framebuffer with a post processing shader.
    // todo: make it not do post processing when this isn't here
    std::optional<Framebuffer> mainFramebuffer;

    // Used for postprocessing, the mesh is just a quad that covers the screen, nothing deep.
    RenderableMesh screenQuad;

    // just a little thing to visualize axis
    void DebugAxis();

    // meshpools are highly optimized objects used for very fast drawing of meshes
    // to avoid memory fragmentation all meshes within it are padded to be of the same size, so to save memory there is a pool for small meshes, medium ones, etc.
    // pools also have to be divided by which shader program and texture/texture array they use
    // this is a map<shaderId, map<materialId, map<poolId, Meshpool*>>>
    // materialId can also == 0 for no material.
    std::unordered_map<unsigned int, std::unordered_map<unsigned int, std::unordered_map<unsigned int, Meshpool*>>> meshpools;
    unsigned long long lastPoolId = 0; 

    // Cache of meshes to add when addCachedMeshes is called. 
    // Used so that instead of adding 1 mesh to a meshpool 1000 times, we just add 1000 instances of a mesh to meshpool once to make creating renderable objects faster.
    // Keys go like meshesToAdd[shaderId][textureId][meshId] = vector of pointers to MeshLocations stored in RenderComponents.
    std::unordered_map<unsigned int, std::unordered_map<unsigned int, std::unordered_map<unsigned int, std::vector<RenderComponent*>>>> renderComponentsToAdd;

    // Keeps track of rendercomponents whose instanced vertex attributes (color, textureZ, etc.) need to be updated.
    // Each tuple is <componentToUpdate, attributeName, value, timesRemainingToUpdate>.
    std::vector<std::tuple<RenderComponent*, unsigned int, glm::vec4, unsigned int>> Instanced4ComponentVertexAttributeUpdates;
    std::vector<std::tuple<RenderComponent*, unsigned int, glm::vec3, unsigned int>> Instanced3ComponentVertexAttributeUpdates;
    std::vector<std::tuple<RenderComponent*, unsigned int, glm::vec2, unsigned int>> Instanced2ComponentVertexAttributeUpdates;
    std::vector<std::tuple<RenderComponent*, unsigned int, glm::vec1, unsigned int>> Instanced1ComponentVertexAttributeUpdates;

    GraphicsEngine();
    ~GraphicsEngine();

    void UpdateMainFramebuffer();
    void CalculateLightingClusters();
    void UpdateLights();
    void DrawSkybox();
    void Update();
    void UpdateRenderComponents(float dt);

    // updates the freecam based off user input (WASD and mouse)
    void UpdateDebugFreecam();
    void AddCachedMeshes();
    void UpdateMeshpools();
    // postProc is true if what's being drawn SHOULD do post proc
    void DrawWorld(bool postProc);

    // void SetColor(const RenderComponent& component, const glm::vec4& rgba);
    void SetModelMatrix(const RenderComponent& component, const glm::mat4x4& model);
    void SetNormalMatrix(const RenderComponent& component, const glm::mat3x3& normal);
    void SetBoneState(const RenderComponent& component, unsigned int nBones, glm::mat4x4* boneTransforms);

    // void SetArbitrary1(const RenderComponent& component, const float arb);

    // set to -1.0 for no texture
    // void SetTextureZ(const RenderComponent& component, const float textureZ);

    // Returns a drawId used to modify the mesh later on.
    // Does not actually add the object for performance reasons, just puts it on a list of stuff to add when GraphicsEngine::addCachedMeshes is called.
    // Contents of MeshLocation are undefined until addCachedMeshes() is called, except for textureId, shaderProgramId, and initialized.
    // Before addChachedMeshes is called, meshLocation->initialized == false and after == true
    void AddObject(unsigned int shaderId, unsigned int materialId, unsigned int meshId, RenderComponent* component);
};

