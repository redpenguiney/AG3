#pragma once
#include "../../external_headers/GLEW/glew.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <tuple>
#include<vector>
#include<atomic>
#include <set>
#include <unordered_map>
#include <memory>
#include "../../external_headers/GLM/ext.hpp"
#define TINYOBJLOADER_IMPLEMENTATION // what kind of library makes you have to ask it to actually implement the functions???
#include "../../external_headers/tinyobjloader/tiny_obj_loader.h"
#include "let_me_hash_a_tuple.cpp"

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
                                      // NOTE: if you're constantly adding and removing unique meshes, they better all have same expectedCount or it's gonna leak memory
    const glm::vec3 originalSize; // TODO

    // engine calls this to get mesh from an object's meshId
    static std::shared_ptr<Mesh>& Get(int meshId) {
        assert(LOADED_MESHES.count(meshId) != 0 && "Mesh::Get() was given an invalid meshId.");
        return LOADED_MESHES[meshId];
    }

    // returns mesh id of the generated mesh
    // verts must be organized into (XYZ, TextureXY, NormalXYZ, RGBA if !instanceColor, TextureZ if !instanceTextureZ).
    // leave expectedCount at 1024 unless it's something like a cube, in which case set it to like 1 million (you can create more objects than this number, it just might lag a little)
    static unsigned int FromVertices(std::vector<GLfloat> &verts, std::vector<GLuint> &indies, bool instanceColor=true, bool instanceTextureZ=true, unsigned int expectedCount=1024) {
        unsigned int meshId = LAST_MESH_ID; // (creating a mesh increments this)
        LOADED_MESHES[meshId] = std::shared_ptr<Mesh>(new Mesh(verts, indies, instanceColor, instanceTextureZ, expectedCount));
        return meshId;
    }

    // returns mesh id of the generated mesh.
    // only accepts OBJ files.
    // File should just contain one object. 
    // TODO: materials
    // TODO: MTL support should be easy
    // meshTransparency will be the initial alpha value of every vertex color, because obj files only support RGB (and also they don't REALLY support RGA)
    static unsigned int FromFile(const std::string& path, bool instanceTextureZ=true, bool instanceColor=true, float textureZ=-1.0, unsigned int transparency=1.0, unsigned int expectedCount = 1024) {
        // Load file
        auto config = tinyobj::ObjReaderConfig();
        config.triangulate = true;
        config.vertex_color = !instanceColor;
        bool success = OBJ_LOADER.ParseFromFile(path, config);
        if (!success) {
            std::printf("Mesh::FromFile failed to load %s because %s", path.c_str(), OBJ_LOADER.Error().c_str());
            abort();
        }

        const unsigned int nFloatsPerVertex = 3 + 3 + 2 + (instanceTextureZ ? 0 : 1) + (instanceColor ? 0 : 4);

        auto shape = OBJ_LOADER.GetShapes().at(0);
        //auto material = OBJ_LOADER.GetMaterials().at(0);
        auto attrib = OBJ_LOADER.GetAttrib();

        std::vector<GLfloat> & positions = attrib.vertices;
        std::vector<GLfloat> & texcoordsXY = attrib.texcoords;
        std::vector<GLfloat> & normals = attrib.normals;
        std::vector<GLfloat> & colors = attrib.colors;

        // ngl im just doing what https://stackoverflow.com/questions/36447021/obtain-indices-from-obj says here
        std::unordered_map<std::tuple<GLuint, GLuint, GLuint>, GLuint, hash_tuple::hash<std::tuple<GLuint, GLuint, GLuint>>> objIndicesToGlIndices; // tuple is (posIndex, texXyIndex, normalIndex)
        std::vector<GLuint> indices;
        std::vector<GLfloat> vertices;
        for (auto & index : shape.mesh.indices) {
            auto indexTuple = std::make_tuple(index.vertex_index, index.texcoord_index, index.normal_index);
            //std::printf("\nindex tuple %d %d %d, %d vertices", index.vertex_index, index.texcoord_index, index.normal_index, vertices.size());
            if (objIndicesToGlIndices.count(indexTuple) == 0) {

                objIndicesToGlIndices[indexTuple] = vertices.size()/nFloatsPerVertex;
                indices.push_back(vertices.size()/nFloatsPerVertex);

                vertices.push_back(positions[index.vertex_index * 3]);
                vertices.push_back(positions[index.vertex_index * 3 + 1]);
                vertices.push_back(positions[index.vertex_index * 3 + 2]);
                
                vertices.push_back(texcoordsXY[index.texcoord_index * 2]); 
                vertices.push_back(texcoordsXY[index.texcoord_index * 2 + 1]);
                
                vertices.push_back(normals[index.normal_index * 3]);
                vertices.push_back(normals[index.normal_index * 3 + 1]);
                vertices.push_back(normals[index.normal_index * 3 + 2]);

                if (!instanceColor) {
                    vertices.push_back(colors[index.vertex_index * 3]);
                    vertices.push_back(colors[index.vertex_index * 3 + 1]);
                    vertices.push_back(colors[index.vertex_index * 3+ 2]);
                    vertices.push_back(transparency);
                }

                if (!instanceTextureZ) {
                    vertices.push_back(textureZ);
                }
            }
            else {
                indices.push_back(objIndicesToGlIndices[indexTuple]);
            }
        }
        
        unsigned int meshId = LAST_MESH_ID; // (creating a mesh increments this)
        LOADED_MESHES[meshId] = std::shared_ptr<Mesh>(new Mesh(vertices, indices, instanceColor, instanceTextureZ, expectedCount));
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
    instanceCount(expectedCount),
    originalSize()
    {}

    inline static std::atomic<unsigned int> LAST_MESH_ID = {0};
    inline static std::unordered_map<unsigned int, std::shared_ptr<Mesh>> LOADED_MESHES; 
    inline static tinyobj::ObjReader OBJ_LOADER;
};

// key is mesh id, value is ptr to mesh
// gameobjects store their mesh id, the engine uses this map to get the actual mesh from that id
// if you are dynamically creating and removing meshes, it is completely safe to remove them from this hashma
