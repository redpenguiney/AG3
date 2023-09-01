#pragma once
#include "mesh.cpp"
#include "meshpool.cpp"
#include "window.cpp"
#include <unordered_map>

struct MeshLocation {
    unsigned int meshType; 
    unsigned int poolId; // uuid of the meshpool 
    unsigned int poolSlot;
    unsigned int poolInstance;
};

class GraphicsEngine {
    public:
    GraphicsEngine()
    : window(500, 500) {
        
    }

    ~GraphicsEngine() {

    }

    // returns true if the user is trying to close the application, or if glfwSetWindowShouldClose was explicitly called (like by a quit game button)
    bool ShouldClose() {
        return window.ShouldClose();
    }

    // Draws everything
    void RenderScene() {
        update();
    }

    // Does not actually add the object for performance reasons, just puts it on a list of stuff to add when GraphicsEngine::addCachedMeshes is called
    void AddObject() {

    }

    private:
    Window window; // handles windowing

    std::unordered_map<unsigned int, Meshpool> meshpools;
    // meshpools are highly optimized objects used for very fast drawing of meshes
    // to avoid memory fragmentation all meshes within it are padded to be of the same size, so to save memory there is a pool for small meshes, medium ones, etc.

    void update() {
        window.Update();
        addCachedMeshes();
    }

    void addCachedMeshes() {

    }
};

GraphicsEngine GE;