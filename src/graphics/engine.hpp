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
#include "../gameobjects/component_pool.hpp"
#include "../gameobjects/transform_component.cpp"
#include "../utility/utility.hpp"
#include "texture.hpp"
#include "renderable_mesh.hpp"

struct MeshLocation {
    unsigned int poolId; // uuid of the meshpool 
    unsigned int poolSlot;
    unsigned int poolInstance;
    unsigned int textureId;
    unsigned int shaderProgramId;
    bool initialized;
    MeshLocation() {
        initialized = false;
    }
};

// Handles graphics, obviously.
class GraphicsEngine {
    public:
    // the shader used to render the skybox. Can freely change this with no issues.
    std::shared_ptr<ShaderProgram> skyboxShaderProgram;
    // skybox texture. Must be a cubemap texture.
    std::shared_ptr<Texture> skyboxTexture;

    // freecam is just a thing for debugging
    bool debugFreecamEnabled = false;
    glm::dvec3 debugFreecamPos = {0, 0, 0};
    double debugFreecamPitch = 0; // in degrees, don't get tripped up when you do lookvector which wants radians
    double debugFreecamYaw = 0;
    double debugFreecamSpeed = 0;
    static const inline double debugFreecamAcceleration = 0.01;

    // Be advised. If you're tryna get camera position/orientation while using debugfreecam, you might want to use debugFreecamPitch/Yaw/Pos instead
    Camera camera;
    Window window = Window(500, 500); // handles windowing, interfaces with GLFW in general

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
    class RenderComponent: BaseComponent<RenderComponent> {
        public:

        // not const because object pool, don't actually change this
        unsigned int meshId;

        // DO NOT delete this pointer.
        static RenderComponent* New(unsigned int mesh_id, unsigned int texture_id, bool visible = true, unsigned int shader_id = Get().defaultShaderProgramId);

        // call instead of deleting the pointer.
        // obviously don't touch component after this.
        void Destroy();

        // We have both GraphicsEngine::SetColor() and RenderComponent::SetColor() because people may want to set color immediately, but GraphicsEngine::SetColor()
        //      needs the meshLocation to be initialized and because we cache mesh creation that doesn't happen until RenderScene() is called.
        void SetColor(glm::vec4 rgba);
        void SetTextureZ(float textureZ);

        private:

        // TODO: getters for color and textureZ
        glm::vec4 color;
        float textureZ;

        // We want to avoid sending color/textureZ to the gpu every frame if we only change it once, but due to multiple buffering we got to set it for every buffer sub section, doing one each frame. Value is the amount of frames we need to set color/textureZ for still.
        // -1 if not instanced
        int colorChanged;
        int textureZChanged;

        MeshLocation meshLocation;
        friend class GraphicsEngine;
        
        //private constructor to enforce usage of object pool
        friend class ComponentPool<RenderComponent>;
        RenderComponent();
    };

    private:
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

    unsigned int defaultShaderProgramId;

    // meshpools are highly optimized objects used for very fast drawing of meshes
    // to avoid memory fragmentation all meshes within it are padded to be of the same size, so to save memory there is a pool for small meshes, medium ones, etc.
    // pools also have to be divided by which shader program and texture/texture array they use
    // this is a map<shaderId, map<textureId, map<poolId, Meshpool*>>>
    std::unordered_map<unsigned int, std::unordered_map<unsigned int, std::unordered_map<unsigned int, Meshpool*>>> meshpools;
    unsigned long long lastPoolId = 0; 

    // tells how to get to the meshpool data of an object from its drawId
    //std::unordered_map<unsigned long long, MeshLocation> drawIdPoolLocations;
    //unsigned long long lastDrawId = 0;

    ComponentPool<RenderComponent> RENDER_COMPONENTS;

    // Cache of meshes to add when addCachedMeshes is called. 
    // Used so that instead of adding 1 mesh to a meshpool 1000 times, we just add 1000 instances of a mesh to meshpool once to make creating renderable objects faster.
    // Keys go like meshesToAdd[shaderId][textureId][meshId] = vector of pointers to MeshLocations stored in RenderComponents.
    std::unordered_map<unsigned int, std::unordered_map<unsigned int, std::unordered_map<unsigned int, std::vector<MeshLocation*>>>> meshesToAdd;

    GraphicsEngine();
    ~GraphicsEngine();

    void CalculateLightingClusters();
    void UpdateLights();
    void DrawSkybox();
    void Update();
    void UpdateRenderComponents();

    // updates the freecam based off user input (WASD and mouse) and then returns a camera matrix
    glm::mat4x4 UpdateDebugFreecam();
    void AddCachedMeshes();
    void UpdateMeshpools();

    void SetColor(MeshLocation& location, glm::vec4 rgba);
    void SetModelMatrix(MeshLocation& location, glm::mat4x4 model);

    // set to -1.0 for no texture
    void SetTextureZ(MeshLocation& location, float textureZ);

    // Returns a drawId used to modify the mesh later on.
    // Does not actually add the object for performance reasons, just puts it on a list of stuff to add when GraphicsEngine::addCachedMeshes is called.
    // Contents of MeshLocation are undefined until addCachedMeshes() is called, except for textureId, shaderProgramId, and initialized.
    // Before addChachedMeshes is called, meshLocation->initialized == false and after == true
    void AddObject(unsigned int shaderId, unsigned int textureId, unsigned int meshId, MeshLocation* meshLocation);
};

