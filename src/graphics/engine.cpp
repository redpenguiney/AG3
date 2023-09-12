#pragma once
#include "mesh.cpp"
#include "meshpool.cpp"
#include "window.cpp"
#include "shader_program.cpp"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>
#include "../debug/debug.cpp"
#include "camera.cpp"
#include "../gameobjects/component_pool.cpp"
#include "../gameobjects/transform_component.cpp"

const unsigned int RENDER_COMPONENT_POOL_SIZE = 65536;

struct MeshLocation {
    unsigned int poolId; // uuid of the meshpool 
    unsigned int poolSlot;
    unsigned int poolInstance;
    bool initialized;
    MeshLocation() {
        initialized = false;
    }
};

class GraphicsEngine {
    public:
    
    Camera camera;

    GraphicsEngine()
    : window(500, 500),
    worldShader("shaders/world_vertex.glsl", "shaders/world_fragment.glsl") {
        
    }

    ~GraphicsEngine() {
        for (auto & [id, pool] : meshpools) {
            delete pool;
        }
    }

    // returns true if the user is trying to close the application, or if glfwSetWindowShouldClose was explicitly called (like by a quit game button)
    bool ShouldClose() {
        return window.ShouldClose();
    }

    // passes projection and camera matrices to shader
    void SetCameraUniforms() {
        worldShader.UniformMat4x4("proj", camera.GetProj((float)window.width/(float)window.height), false);
        worldShader.UniformMat4x4("camera", camera.GetCamera(), false);
    }

    // Draws everything
    void RenderScene() {
        update();

        SetCameraUniforms();
        worldShader.Use();
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST); // stuff near the camera should be drawn over stuff far from the camera
        //glEnable(GL_CULL_FACE); // backface culling
        for (auto & [_, pool] : meshpools) {
            pool->Draw();

        } 


        window.Update(); // this includes flipping the buffer so it goes at the end
    }

    // duh
    void SetColor(MeshLocation& location, glm::vec4 rgba) {
        assert(location.initialized);
        meshpools[location.poolId]->SetColor(location.poolSlot, location.poolInstance, rgba);
    }

    // duh
    void SetModelMatrix(MeshLocation& location, glm::mat4x4 model) {
        assert(location.initialized);
        meshpools[location.poolId]->SetModelMatrix(location.poolSlot, location.poolInstance, model);
    }

    // Gameobjects that want to be rendered should have a pointer to one of these.
    // However, they are stored here in a vector because that's better for the cache. (google ECS).
    // NEVER DELETE THIS POINTER, JUST CALL Destroy(). DO NOT STORE OUTSIDE A GAMEOBJECT. THESE USE AN OBJECT POOL.
    class RenderComponent {
        public:
        // because object pool, instances of this struct might just be uninitialized memory
        // also, gameobjects have to reserve a render component even if they don't use one (see gameobject.cpp), so they can set this to false if they don't actually want one
        bool live;
        unsigned int meshId;

        static RenderComponent* New(unsigned int mesh_id) {
            
            auto ptr = RENDER_COMPONENTS.GetNew();
            ptr->live = true;
            ptr->meshId = mesh_id;
            addObject(ptr->meshId, &ptr->meshLocation);
            return ptr;

        };

        // call instead of deleting the pointer.
        // obviously don't touch component after this.
        void Destroy() {
            assert(live == true);

            // if some pyschopath created a RenderComponent and then instantly deleted it, we need to remove it from GraphicsEngine::meshesToAdd
            if (!meshLocation.initialized) { 
                auto & vec = meshesToAdd.at(meshId);
                int index = -1;
                for (auto & ptr : vec) {
                    if (ptr == &meshLocation) {
                        break;
                    }
                    index++;
                }
                vec.erase(vec.begin() + index);
            }
            else { // otherwise just remove object from graphics engine
                // TODO
            }

            live = false;
            
            RENDER_COMPONENTS.ReturnObject(this);
        }

        // this union exists so we can use a "free list" memory optimization, see component_pool.cpp
        union {
            // live state
            //struct {
                // DO NOT TOUCH, ONLY FOR ENGINE
                MeshLocation meshLocation;
            //};

            //dead state
            struct {
                RenderComponent* next; // pointer to next available component in pool
                unsigned int componentPoolId; // index into pools vector
            };
            
        };

        private:
        //private constructor to enforce usage of object pool
        friend class ComponentPool<RenderComponent, RENDER_COMPONENT_POOL_SIZE>;
        RenderComponent() {
            live = false;
        }
    };

    private:
    Window window; // handles windowing
    ShaderProgram worldShader; // everything is drawn with this shader

    // meshpools are highly optimized objects used for very fast drawing of meshes
    // to avoid memory fragmentation all meshes within it are padded to be of the same size, so to save memory there is a pool for small meshes, medium ones, etc.
    static inline std::unordered_map<unsigned int, Meshpool*> meshpools; // didn't want to use raw pointers but it's the only way it seems
    static inline unsigned long long lastPoolId = 0; 

    // tells how to get to the meshpool data of an object from its drawId
    //std::unordered_map<unsigned long long, MeshLocation> drawIdPoolLocations;
    //inline static unsigned long long lastDrawId = 0;


    static inline ComponentPool<RenderComponent, RENDER_COMPONENT_POOL_SIZE> RENDER_COMPONENTS;

    // Cache of meshes to add when addCachedMeshes is called. 
    // Used so that instead of adding 1 mesh to a meshpool 1000 times, we just add 1000 instances of a mesh to meshpool once to make creating renderable objects faster.
    // Key is meshId, value is vector of pointers to MeshLocations stored in RenderComponents.
    static inline std::unordered_map<unsigned int, std::vector<MeshLocation*>> meshesToAdd;

    void update() {  
        addCachedMeshes();
        updateRenderComponents();        
    }

    void updateRenderComponents() {
        for (unsigned int i = 0; i < RENDER_COMPONENTS.pools.size(); i++) {
            auto renderArray = RENDER_COMPONENTS.pools.at(i);
            auto transformArray = TransformComponent::TRANSFORM_COMPONENTS.pools.at(i);
            for (unsigned int j = 0; j < RENDER_COMPONENT_POOL_SIZE; j++) {
                auto renderComp = renderArray + j;
                auto transformComp = transformArray + j;
                if (renderComp->live) {
                    SetModelMatrix(renderComp->meshLocation, transformComp->GetModel(camera.position));
                }
            }
        }
    }

    void addCachedMeshes() {
        for (auto & [meshId, meshLocations] : meshesToAdd) {
            
            std::shared_ptr<Mesh>& m = Mesh::Get(meshId);
            const unsigned int verticesNBytes = m->vertices.size() * sizeof(GLfloat);
            const unsigned int indicesNBytes = m->indices.size() * sizeof(GLuint);
            const bool shouldInstanceColor = m->instancedColor;
            const bool shouldInstanceTextureZ = m->instancedTextureZ;

            // pick best pool for mesh
            int bestPoolScore = INT_MAX;
            int bestPoolId = -1;
            for (auto & [poolId, pool] : meshpools) {
                int score = pool->ScoreMeshFit(verticesNBytes, indicesNBytes, shouldInstanceColor, shouldInstanceTextureZ);
                if (score == -1) {continue;} // this continues the inner loop which is what we want
                if (score < bestPoolScore) {
                    bestPoolScore = score;
                    bestPoolId = poolId;
                }
            }

            if (bestPoolId == -1) { // if we didn't find a suitable pool just make one
                bestPoolId = lastPoolId++;
                meshpools[bestPoolId] = new Meshpool(m);
            }

            auto objectPositions = meshpools[bestPoolId]->AddObject(meshId, meshLocations.size());
            for (unsigned int i = 0; i < meshLocations.size(); i++) {
                meshLocations[i]->poolId = bestPoolId;
                meshLocations[i]->poolSlot = objectPositions[i].first;
                meshLocations[i]->poolInstance = objectPositions[i].second;
                meshLocations[i]->initialized = true;
            }
        } 

        meshesToAdd.clear();
    }

    // Returns a drawId used to modify the mesh later on.
    // Does not actually add the object for performance reasons, just puts it on a list of stuff to add when GraphicsEngine::addCachedMeshes is called.
    // Contents of MeshLocation pointer are undefined until addCachedMeshes() is called.
    static void addObject(unsigned int meshId, MeshLocation* meshLocation) {
        meshesToAdd[meshId].push_back(meshLocation);
    }
};

GraphicsEngine GE;