#pragma once
#include "../../external_headers/GLM/ext.hpp"
#include "../../external_headers/GLEW/glew.h"
#include <vector>
#include <set>
#include <unordered_map>
#include <array>
#include <tuple>
#include <memory>
#include <atomic>
#include <string>
#include "../../external_headers/tinyobjloader/tiny_obj_loader.h"

// TODO: MODIFY MESH
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
                                      // NOTE: if you're constantly adding and removing unique meshes, they better all have same expectedCount or it's gonna cost you in memory
    const glm::vec3 originalSize; // When loading a mesh from file, it is automatically scaled so all vertex positions are in the range -0.5 to 0.5. (this lets you and the physics engine easily know what the actual size of the object is) Set gameobject scale to this value to restore it to original size.

    static std::shared_ptr<Mesh>& Get(unsigned int meshId);
    static std::shared_ptr<Mesh> FromVertices(std::vector<GLfloat> &verts, std::vector<GLuint> &indies, bool instanceColor=true, bool instanceTextureZ=true, unsigned int expectedCount=1024);
    static std::shared_ptr<Mesh> FromFile(const std::string& path, bool instanceTextureZ=true, bool instanceColor=true, float textureZ=-1.0, unsigned int transparency=1.0, unsigned int expectedCount = 1024);
    static void Unload(int meshId);

    private:
    Mesh(std::vector<GLfloat> &verts, std::vector<GLuint> &indies, bool instanceColor, bool instanceTextureZ, unsigned int expectedCount);
    inline static std::atomic<unsigned int> LAST_MESH_ID = {0};
    inline static std::unordered_map<unsigned int, std::shared_ptr<Mesh>> LOADED_MESHES; 
    inline static tinyobj::ObjReader OBJ_LOADER;

};