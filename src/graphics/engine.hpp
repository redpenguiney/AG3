#pragma once
#include "mesh.hpp"
#include "meshpool.hpp"
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

// Handles graphics, obviously.
class GraphicsEngine {
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

    // freecam is just a thing for debugging
    bool debugFreecamEnabled = false;
    glm::dvec3 debugFreecamPos = {0, 0, 0};
    double debugFreecamPitch = 0; // in degrees, don't get tripped up when you do lookvector which wants radians
    double debugFreecamYaw = 0;
    double debugFreecamSpeed = 0;
    static const inline double debugFreecamAcceleration = 0.01;

    // Be advised. If you're tryna get camera position/orientation while using debugfreecam, you might want to use debugFreecamPitch/Yaw/Pos instead
    Camera camera;
    Window window = Window(720, 720); // handles windowing, interfaces with GLFW in general

    static GraphicsEngine& Get();

    // Used by Texture to throw an error if someone tries to unload a texture being used
    bool IsTextureInUse(unsigned int textureId);

    // Used by ShaderProgram to throw an error if someone tries to unload a shader being used.
    bool IsShaderProgramInUse(unsigned int shaderId);

    // returns true if the user is trying to close the application, or if glfwSetWindowShouldClose was explicitly called (like by a quit game button)
    bool ShouldClose();

    // Draws everything
    void RenderScene();

    // Gameobjects that want to be rendered should have a pointer to one of these.
    // However, they are stored here in a vector because that's better for the cache. (google ECS).
    // NEVER DELETE THIS POINTER, JUST CALL Destroy(). DO NOT STORE OUTSIDE A GAMEOBJECT. THESE USE AN OBJECT POOL.
    class RenderComponent: public BaseComponent<RenderComponent> {
        public:

        // not const because object pool, don't actually change this
        unsigned int meshId;
        unsigned int shaderProgramId, materialId;

        // called to initialize when given to a gameobject
        void Init(unsigned int mesh_id, unsigned int materialId, unsigned int shader_id = Get().defaultShaderProgram->shaderProgramId);

        // call before returning to pool
        void Destroy();

        // We have both GraphicsEngine::SetColor() and RenderComponent::SetColor() because people may want to set color immediately, but GraphicsEngine::SetColor()
        //      expects the meshLocation to be initialized and because we cache mesh creation that doesn't happen until RenderScene() is called.
        void SetColor(const glm::vec4& rgba);
        void SetTextureZ(const float textureZ);

        private:

        // TODO: getters for color and textureZ
        glm::vec4 color;
        float textureZ;

        // We want to avoid sending color/textureZ to the gpu every frame if we only change it once, but due to multiple buffering we got to set it for every buffer sub section, doing one each frame. Value is the amount of frames we need to set color/textureZ for still.
        // -1 if not instanced
        int colorChanged;
        int textureZChanged;

        // MeshLocation meshLocation;
        

        // -1 before being initialized
        int meshpoolId, meshpoolSlot, meshpoolInstance;

        friend class GraphicsEngine;

        // mesh.cpp needs to access mesh location sorry
        friend class Mesh;
        
        //private constructor to enforce usage of object pool
        friend class ComponentPool<RenderComponent>;
        RenderComponent();
    };

    // Identical to a RenderComponent in every way, except that it doesn't use floating origin. Not a bool flag on a normal render component bc i like premature optimization.
    class RenderComponentNoFO: public RenderComponent {
        public:
        // TODO: implicit conversion to normal rendercomponent because they do the exact same things?
        
        private:

        //private constructor to enforce usage of object pool
        friend class ComponentPool<RenderComponentNoFO>;
        RenderComponentNoFO();

        // exists to let RenderComponentNoFO avoid type safety.
        // void* should be ptr to component pool
        void SetPool(void* p) {
            pool = (ComponentPool<RenderComponent>*)p;
        }
    };

    static_assert(sizeof(RenderComponent) == sizeof(RenderComponentNoFO), "These classes need the exact same memory layout or i'll be sad.\n");

    std::shared_ptr<Material> GetDefaultMaterial(); // TODO

    // Render components that use a dynamic mesh add themselves to this unordered map (and remove themselves on destruction).
    // Key is mesh id, value is ptr to rendercomponent which uses that mesh.
    // Needed so that mesh.cpp can access the meshpools being used by objects with dynamic meshes, in order to apply changes made to those dynamic meshes to the GPU.
    std::unordered_map<unsigned int, std::vector<RenderComponent*>> dynamicMeshUsers;

    private:

    friend class Mesh; // literally just friend so dynamic mesh support is less work for me idc about keeping it modular

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

    GraphicsEngine();
    ~GraphicsEngine();

    void UpdateMainFramebuffer();
    void CalculateLightingClusters();
    void UpdateLights();
    void DrawSkybox();
    void Update();
    void UpdateRenderComponents();

    // updates the freecam based off user input (WASD and mouse) and then returns a camera matrix
    glm::mat4x4 UpdateDebugFreecam();
    void AddCachedMeshes();
    void UpdateMeshpools();

    void SetColor(const RenderComponent& component, const glm::vec4& rgba);
    void SetModelMatrix(const RenderComponent& component, const glm::mat4x4& model);
    void SetNormalMatrix(const RenderComponent& component, const glm::mat3x3& normal);

    // set to -1.0 for no texture
    void SetTextureZ(const RenderComponent& component, const float textureZ);

    // Returns a drawId used to modify the mesh later on.
    // Does not actually add the object for performance reasons, just puts it on a list of stuff to add when GraphicsEngine::addCachedMeshes is called.
    // Contents of MeshLocation are undefined until addCachedMeshes() is called, except for textureId, shaderProgramId, and initialized.
    // Before addChachedMeshes is called, meshLocation->initialized == false and after == true
    void AddObject(unsigned int shaderId, unsigned int materialId, unsigned int meshId, RenderComponent* component);
};

