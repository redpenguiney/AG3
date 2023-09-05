#pragma once
#include "../../external_headers/GLEW/glew.h"
#include<GL/GL.h>
#include <cassert>
#include<vector>
#include<atomic>
#include <unordered_map>
#include <memory>

class Mesh {
    public:

    const int meshId; // meshes have an id so that the engine can easily determine that two objects use the same mesh and instance them
    const std::vector<GLfloat> vertices;
    const std::vector<GLuint> indices; 
    const bool instancedColor; // If the value is NOT instanced, then each vertex of the mesh can have its own color and look pretty
    const bool instancedTextureZ;
    const unsigned int instanceCount; // meshpool will make room for this many objects to use this mesh (if you go over it's fine, but performance may be affected)
                                      // the memory cost of this is ~64 * instanceCount bytes
                                      // def make it like a million for cubes and stuff, otherwise default of 1024 should be fine
                                      // NOTE: if you're constantly adding and removing unique meshes, they better all have same expectedCount or it's gonna waste a lot of memory

    // engine calls this to get mesh from an object's meshId
    static std::shared_ptr<Mesh>& Get(int meshId) {
        assert(LOADED_MESHES.count(meshId) != 0 && "Mesh::Get() was given an invalid meshId.");
        return LOADED_MESHES[meshId];
    }

    // returns mesh id of the generated mesh
    // verts must be organized into (XYZ, TextureXY, NormalXYZ, RGBA if !instanceColor, TextureZ if !instanceTextureZ)
    // leave expectedCount at 1024 unless it's something like a cube, in which case set it to like 1 million (you can create more objects than this number, it just might lag a little)
    static int FromVertices(std::vector<GLfloat> &verts, std::vector<GLuint> &indies, bool instanceColor=true, bool instanceTextureZ=true, unsigned int expectedCount=1024) {
        int meshId = LAST_MESH_ID;
        LOADED_MESHES[meshId] = std::shared_ptr<Mesh>(new Mesh(verts, indies, instanceColor, instanceTextureZ, expectedCount));
        return meshId;
    }

    // unloads the mesh with the given meshId, freeing its memory.
    // you CAN unload a mesh while objects are using it without any issues - a copy of that mesh is still in a meshpool.
    // you only need to call this function if you are (like for procedural terrain) dynamically loading new meshes
    static void Unload(int meshId) {
        assert(LOADED_MESHES.count(meshId) != 0 && "Mesh::Unload() was given an invalid meshId.");
        LOADED_MESHES.erase(meshId);
    }

    private: 

    Mesh(std::vector<GLfloat> &verts, std::vector<GLuint> &indies, bool instanceColor, bool instanceTextureZ, unsigned int expectedCount):
    meshId(LAST_MESH_ID++),
    vertices(verts),
    indices(indies),
    instancedColor(instanceColor),
    instancedTextureZ(instanceTextureZ),
    instanceCount(expectedCount)
    {}

    inline static std::atomic<int> LAST_MESH_ID = {0};
    inline static std::unordered_map<int, std::shared_ptr<Mesh>> LOADED_MESHES; 
};

// key is mesh id, value is ptr to mesh
// gameobjects store their mesh id, the engine uses this map to get the actual mesh from that id
// if you are dynamically creating and removing meshes, it is completely safe to remove them from this hashma
