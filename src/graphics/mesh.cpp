#pragma once
#include "mesh.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

#define TINYOBJLOADER_IMPLEMENTATION // what kind of library makes you have to ask it to actually implement the functions???
#include "../../external_headers/tinyobjloader/tiny_obj_loader.h"
#include "let_me_hash_a_tuple.cpp"



// engine calls this to get mesh from an object's meshId
std::shared_ptr<Mesh>& Mesh::Get(unsigned int meshId) {
    assert(LOADED_MESHES.count(meshId) != 0 && "Mesh::Get() was given an invalid meshId.");
    return LOADED_MESHES[meshId];
}

// verts must be organized into (XYZ, TextureXY, NormalXYZ, RGBA if !instanceColor, TextureZ if !instanceTextureZ).
// leave expectedCount at 1024 unless it's something like a cube, in which case set it to like 1 million (you can create more objects than this number, it just might lag a little)
std::shared_ptr<Mesh> Mesh::FromVertices(std::vector<GLfloat> &verts, std::vector<GLuint> &indies, bool instanceColor, bool instanceTextureZ, unsigned int expectedCount) {
    unsigned int meshId = LAST_MESH_ID; // (creating a mesh increments this)
    auto ptr = std::shared_ptr<Mesh>(new Mesh(verts, indies, instanceColor, instanceTextureZ, expectedCount));
    LOADED_MESHES[meshId] = ptr;
    return ptr;
}

// only accepts OBJ files.
// File should just contain one object. 
// TODO: materials
// TODO: MTL support should be easy
// meshTransparency will be the initial alpha value of every vertex color, because obj files only support RGB (and also they don't REALLY support RGA)
std::shared_ptr<Mesh> Mesh::FromFile(const std::string& path, bool instanceTextureZ, bool instanceColor, float textureZ, unsigned int meshTransparency, unsigned int expectedCount) {
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

    // scale vertex positions into range -0.5 to 0.5

    // find mesh extents
    float minX = 0, maxX = 0, minY = 0, maxY = 0, minZ = 0, maxZ = 0;
    unsigned int i = 0; 
    for (float v: positions) {
        if (i % 3 == 0) {
            minX = std::min(minX, v);
            maxX = std::max(maxX, v);
        }
        else if (i % 3 == 1) {
            minY = std::min(minY, v);
            maxY = std::max(maxY, v);
        }
        else if (i % 3 == 2) {
            minZ = std::min(minZ, v);
            maxZ = std::max(maxZ, v);
        }
        i++;
    }

    // scale mesh by extents
    i = 0;
    for (float& v: positions) {
        if (i % 3 == 0) {
            v = 1.0*(v-minX)/(maxX-minX) - 0.5; // i don't really know how this bit works i got it from stack overflow and modified it
        }
        else if (i % 3 == 1) {
            v = 1.0*(v-minY)/(maxY-minY) - 0.5; // i don't really know how this bit works i got it from stack overflow and modified it
        }
        else if (i % 3 == 2) {
            v = 1.0*(v-minZ)/(maxZ-minZ) - 0.5; // i don't really know how this bit works i got it from stack overflow and modified it
        }
        i++;
    }

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
                vertices.push_back(colors[index.vertex_index * 3 + 2]);
                vertices.push_back(meshTransparency);
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
    auto ptr = std::shared_ptr<Mesh>(new Mesh(vertices, indices, instanceColor, instanceTextureZ, expectedCount));
    LOADED_MESHES[meshId] = ptr;
    return ptr;
}

// unloads the mesh with the given meshId, freeing its memory.
// you CAN unload a mesh while objects are using it without any issues - a copy of that mesh is still in a meshpool.
// you only need to call this function if you are (like for procedural terrain) dynamically loading new meshes
void Mesh::Unload(int meshId) {
    assert(LOADED_MESHES.count(meshId) != 0 && "Mesh::Unload() was given an invalid meshId.");
    LOADED_MESHES.erase(meshId);
}

Mesh::Mesh(std::vector<GLfloat> &verts, std::vector<GLuint> &indies, bool instanceColor, bool instanceTextureZ, unsigned int expectedCount):
meshId(LAST_MESH_ID++),
vertices(verts),
indices(indies),
instancedColor(instanceColor),
instancedTextureZ(instanceTextureZ),
instanceCount(expectedCount),
originalSize()
{}


