#pragma once
#include "mesh.cpp"
#include "meshpool.cpp"
#include "window.cpp"
#include "shader_program.hpp"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "../debug/debug.cpp"
#include "camera.cpp"
#include "../gameobjects/component_pool.cpp"
#include "../gameobjects/transform_component.cpp"
#include "../utility/utility.cpp"
#include "texture.hpp"
#include "renderable_mesh.cpp"

const unsigned int RENDER_COMPONENT_POOL_SIZE = 65536;

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

// Seperating into header and cpp due to circular dependency, not because i wanted to.
// Handles graphics, obviously.
class GraphicsEngine {
    public:
    // the shader used to render the skybox. Can freely change this with no issues.
    static inline std::shared_ptr<ShaderProgram> skyboxShaderProgram;
    // skybox texture. Must be a cubemap texture.
    static inline std::shared_ptr<Texture> skyboxTexture;

    // freecam is just a thing for debugging
    static inline bool debugFreecamEnabled = false;
    static inline glm::dvec3 debugFreecamPos = {0, 0, 0};
    static inline float debugFreecamPitch = 0;
    static inline float debugFreecamYaw = 0;
    static inline double debugFreecamSpeed = 0;
    static const inline double debugFreecamAcceleration = 0.01;

    static inline Camera camera;
    static inline Window window = Window(500, 500); // handles windowing, interfaces with GLFW in general

    static void Init();
    static void Terminate();

    static bool IsTextureInUse(unsigned int textureId);
    static bool IsShaderProgramInUse(unsigned int shaderId);

    static bool ShouldClose();

    static void RenderScene();

    static void SetColor(MeshLocation& location, glm::vec4 rgba);
    static void SetModelMatrix(MeshLocation& location, glm::mat4x4 model);
    static void SetTextureZ(MeshLocation& location, float textureZ);

    // Gameobjects that want to be rendered should have a pointer to one of these.
    // However, they are stored here in a vector because that's better for the cache. (google ECS).
    // NEVER DELETE THIS POINTER, JUST CALL Destroy(). DO NOT STORE OUTSIDE A GAMEOBJECT. THESE USE AN OBJECT POOL.
    class RenderComponent {
        public:
        // because object pool, instances of this struct might just be uninitialized memory
        // also, gameobjects have to reserve a render component even if they don't use one (see gameobject.cpp), so they can set this to false if they don't actually want one
        bool live;

        // not const becasue object pool
        unsigned int meshId; // NO CHANGING THIS
        unsigned int textureId; // NO CHANGING THIS
        unsigned int shaderId; // NO CHANGING THIS

        static RenderComponent* New(unsigned int mesh_id, unsigned int texture_id, unsigned int shader_id = defaultShaderProgramId);
        void Destroy();

        // this union exists so we can use a "free list" memory optimization, see component_pool.cpp
        union {
            // live state
            MeshLocation meshLocation;

            //dead state
            struct {
                RenderComponent* next; // pointer to next available component in pool
                unsigned int componentPoolId; // index into pools vector
            };
            
        };

        private:
        //private constructor to enforce usage of object pool
        friend class ComponentPool<RenderComponent, RENDER_COMPONENT_POOL_SIZE>;
        RenderComponent();
    };

    private:
    static RenderableMesh* skybox; 

    static inline unsigned int defaultShaderProgramId;

    // meshpools are highly optimized objects used for very fast drawing of meshes
    // to avoid memory fragmentation all meshes within it are padded to be of the same size, so to save memory there is a pool for small meshes, medium ones, etc.
    // pools also have to be divided by which shader program and texture/texture array they use
    // this is a map<shaderId, map<textureId, map<poolId, Meshpool*>>>
    static inline std::unordered_map<unsigned int, std::unordered_map<unsigned int, std::unordered_map<unsigned int, Meshpool*>>> meshpools;
    static inline unsigned long long lastPoolId = 0; 

    // tells how to get to the meshpool data of an object from its drawId
    //std::unordered_map<unsigned long long, MeshLocation> drawIdPoolLocations;
    //inline static unsigned long long lastDrawId = 0;

    static inline ComponentPool<RenderComponent, RENDER_COMPONENT_POOL_SIZE> RENDER_COMPONENTS;

    // Cache of meshes to add when addCachedMeshes is called. 
    // Used so that instead of adding 1 mesh to a meshpool 1000 times, we just add 1000 instances of a mesh to meshpool once to make creating renderable objects faster.
    // Keys go like meshesToAdd[shaderId][textureId][meshId] = vector of pointers to MeshLocations stored in RenderComponents.
    static inline std::unordered_map<unsigned int, std::unordered_map<unsigned int, std::unordered_map<unsigned int, std::vector<MeshLocation*>>>> meshesToAdd;

    static void DrawSkybox();
    static void Update();
    static void UpdateRenderComponents();
    static glm::mat4x4 UpdateDebugFreecam();
    static void AddCachedMeshes();

    static void AddObject(unsigned int shaderId, unsigned int textureId, unsigned int meshId, MeshLocation* meshLocation);
};

