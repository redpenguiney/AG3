#pragma once
#include "mesh.cpp"
#include "meshpool.cpp"
#include "window.cpp"
#include "shader_program.cpp"
#include <algorithm>
#include <cstdio>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>
#include "../debug/debug.cpp"
#include "camera.cpp"

struct MeshLocation {
    unsigned int poolId; // uuid of the meshpool 
    unsigned int poolSlot;
    unsigned int poolInstance;
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

    void SetCameraUniforms() {
        worldShader.UniformMat4x4("proj", camera.GetProj(window.width/window.height), false);
    }

    // Draws everything
    void RenderScene() {
        update();

        SetCameraUniforms();
        worldShader.Use();
        
        for (auto & [poolId, pool] : meshpools) {
            pool->Draw();

        } 


        window.Update(); // this includes flipping the buffer so it goes at the end
    }

    // Returns a drawId used to modify the mesh later on.
    // Does not actually add the object for performance reasons, just puts it on a list of stuff to add when GraphicsEngine::addCachedMeshes is called
    unsigned long long AddObject(unsigned int meshId) {
        auto drawId = lastDrawId++;
        meshesToAdd[meshId].push_back(drawId);
        drawIdPoolLocations[meshId] = MeshLocation {};
        return drawId;
    }

    private:
    Window window; // handles windowing
    ShaderProgram worldShader; // everything is drawn with this shader

    // meshpools are highly optimized objects used for very fast drawing of meshes
    // to avoid memory fragmentation all meshes within it are padded to be of the same size, so to save memory there is a pool for small meshes, medium ones, etc.
    std::unordered_map<unsigned int, Meshpool*> meshpools; // didn't want to use raw pointers but it's the only way it seems
    inline static unsigned long long lastPoolId = 0; 

    // tells how to get to the meshpool data of an object from its drawId
    std::unordered_map<unsigned long long, MeshLocation> drawIdPoolLocations;
    inline static unsigned long long lastDrawId = 0;

    // Cache of meshes to add when addCachedMeshes is called. 
    // Used so that instead of adding 1 mesh to a meshpool 1000 times, we just add 1000 instances of a mesh to meshpool once to make creating renderable objects faster.
    // Key is meshId, value is vector of drawIds.
    std::unordered_map<unsigned int, std::vector<unsigned long long>> meshesToAdd;
    

    void update() {  
        addCachedMeshes();
    }

    void addCachedMeshes() {
        for (auto & [meshId, drawIds] : meshesToAdd) {
            
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

            auto objectPositions = meshpools[bestPoolId]->AddObject(meshId, drawIds.size());
            for (unsigned int i = 0; i < drawIds.size(); i++) {
                drawIdPoolLocations[i].poolId = bestPoolId;
                drawIdPoolLocations[i].poolSlot = objectPositions[i].first;
                drawIdPoolLocations[i].poolInstance = objectPositions[i].second;
            }
        } 

        meshesToAdd.clear();
    }
};

GraphicsEngine GE;