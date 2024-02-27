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
    
    
    const unsigned int vertexSize; // the size, in bytes, of a single vertex. 
    // TODO
    // const int positionOffset; // the offset, in bytes, of the position vertex attribute (-1 if no such attribute)
    // const int UVOffset; // the offset, in bytes, of the texture UV vertex attribute (-1 if no such attribute)
    

    static std::shared_ptr<Mesh>& Get(unsigned int meshId);

    // verts must be organized into (XYZ, TextureXY, NormalXYZ, TangentXYZ, RGBA if !instanceColor, TextureZ if !instanceTextureZ).
    // leave expectedCount at 1024 unless it's something like a cube, in which case maybe set it to like 100k (you can create more objects than this number, just for instancing)
    static std::shared_ptr<Mesh> FromVertices(std::vector<GLfloat> &verts, std::vector<GLuint> &indies, bool instanceColor=true, bool instanceTextureZ=true, unsigned int expectedCount=1024);

    // only accepts OBJ files.
    // File should just contain one object. 
    // TODO: materials
    // TODO: MTL support should be easy
    // meshTransparency will be the initial alpha value of every vertex color, because obj files only support RGB (and also they don't REALLY support RGA).
    // leave expectedCount at 1024 unless it's something like a cube, in which case maybe set it to like 100k (you can create more objects than this number, just for instancing)
    // normalizeSize should ALWAYS be true unless you're creating a mesh (like the mesh) for internal usage
    static std::shared_ptr<Mesh> FromFile(const std::string& path, bool instanceTextureZ=true, bool instanceColor=true, float textureZ=-1.0, unsigned int transparency=1.0, unsigned int expectedCount = 1024, bool normalizeSize = true);
    static void Unload(int meshId);

    private:
    Mesh(std::vector<GLfloat> &verts, std::vector<GLuint> &indies, bool instanceColor, bool instanceTextureZ, unsigned int expectedCount);
    inline static std::atomic<unsigned int> LAST_MESH_ID = {1};
    inline static std::unordered_map<unsigned int, std::shared_ptr<Mesh>> LOADED_MESHES; 
    inline static tinyobj::ObjReader OBJ_LOADER;

};