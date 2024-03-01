#pragma once
#include "mesh.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "../../external_headers/GLM/gtc/type_ptr.hpp"
#define TINYOBJLOADER_IMPLEMENTATION // what kind of library makes you have to ask it to actually implement the functions???
#include "../../external_headers/tinyobjloader/tiny_obj_loader.h"
#include "../utility/let_me_hash_a_tuple.cpp"



// engine calls this to get mesh from an object's meshId
std::shared_ptr<Mesh>& Mesh::Get(unsigned int meshId) {
    assert(LOADED_MESHES.count(meshId) != 0 && "Mesh::Get() was given an invalid meshId.");
    return LOADED_MESHES[meshId];
}

// // Scales the vertex positions by the given 
// glm::vec3 NormalizeVertices(std::vector<GLfloat> & verts) {

// }

std::shared_ptr<Mesh> Mesh::FromVertices(std::vector<GLfloat> verts, const std::vector<GLuint> &indies, bool instanceColor, bool instanceTextureZ, unsigned int expectedCount, bool normalizeSize) {
    unsigned int meshId = LAST_MESH_ID; // (creating a mesh increments this)
    if (normalizeSize) {
        std::cout << "YOU DIDN\'T ADD THIS YET (MESH.CPP)\n";
        abort();
    }
    auto ptr = std::shared_ptr<Mesh>(new Mesh(verts, indies, instanceColor, instanceTextureZ, expectedCount));
    LOADED_MESHES[meshId] = ptr;
    return ptr;
}

std::shared_ptr<Mesh> Mesh::FromFile(const std::string& path, bool instanceTextureZ, bool instanceColor, float textureZ, unsigned int meshTransparency, unsigned int expectedCount, bool normalizeSize) {
    // Load file
    auto config = tinyobj::ObjReaderConfig();
    config.triangulate = true;
    config.vertex_color = !instanceColor;
    bool success = OBJ_LOADER.ParseFromFile(path, config);
    if (!success) {
        std::printf("Mesh::FromFile failed to load %s because %s", path.c_str(), OBJ_LOADER.Error().c_str());
        abort();
    }

    const unsigned int nFloatsPerVertex = 3 + 3 + 2 + 3 + (instanceTextureZ ? 0 : 1) + (instanceColor ? 0 : 4);

    auto shape = OBJ_LOADER.GetShapes().at(0);
    //auto material = OBJ_LOADER.GetMaterials().at(0);
    auto attrib = OBJ_LOADER.GetAttrib();

    std::vector<GLfloat> & positions = attrib.vertices;
    std::vector<GLfloat> & texcoordsXY = attrib.texcoords;
    std::vector<GLfloat> & normals = attrib.normals;
    std::vector<GLfloat> & colors = attrib.colors;

    // scale vertex positions into range -0.5 to 0.5

    if (normalizeSize) {
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
    }
    

    // take all the seperate colors, positions, etc. and put them in a single interleaved vertices thing with one indices
    std::unordered_map<std::tuple<GLuint, GLuint, GLuint>, GLuint, hash_tuple::hash<std::tuple<GLuint, GLuint, GLuint>>> objIndicesToGlIndices; // tuple is (posIndex, texXyIndex, normalIndex)
    std::vector<GLuint> indices;
    std::vector<GLfloat> vertices;
    for (auto & index : shape.mesh.indices) {
        auto indexTuple = std::make_tuple(index.vertex_index, index.texcoord_index, index.normal_index);

        // if we have this exact vertex already, just add another index for it, otherwise append a new vertex
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

            // tangent vectors are calculated on next step
            vertices.push_back(0);
            vertices.push_back(0);
            vertices.push_back(0);

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

    // calculate tangent vectors
    std::vector<GLfloat> tangents;
    // const unsigned int sizeOfVertex = sizeof(glm::vec3) + sizeof(glm::vec3) + sizeof(glm::vec3) + sizeof(glm::vec2) + ((!instanceColor) ? sizeof(glm::vec4) : 0) + ((!instanceTextureZ) ? sizeof(GLfloat) : 0);
    //std::cout << "There are " << vertices.size() << " vertices.\n";
    
    for (unsigned int triangleIndex = 0; triangleIndex < indices.size()/3; triangleIndex++) {
        glm::vec3 points[3];
        glm::vec3 texCoords[3];
        glm::vec3 l_normals[3]; // l_ to avoid shadowing
        for (unsigned int j = 0; j < 3; j++) {
            auto indexIntoIndices = triangleIndex * 3 + j;
            //std::cout << " i = " << indexIntoIndices << "nfloats = " << nFloatsPerVertex << " actual index = " << indices.at(indexIntoIndices) <<  " \n";
            auto vertexIndex = indices.at(indexIntoIndices);
            //std::cout << "thing in indices  was " << vertexIndex << " \n"; 
            points[j] = glm::make_vec3(&vertices.at(nFloatsPerVertex * vertexIndex));
            texCoords[j] = glm::make_vec3(&vertices.at(nFloatsPerVertex * vertexIndex + 3));
            l_normals[j] = glm::make_vec3(&vertices.at(nFloatsPerVertex * vertexIndex + 5));
        }
         
        glm::vec3 edge1 = points[1] - points[0];
        glm::vec3 edge2 = points[2] - points[0];
        glm::vec2 deltaUV1 = texCoords[1] - texCoords[0];
        glm::vec2 deltaUV2 = texCoords[2] - texCoords[0]; 
  
        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);


        auto atangentX = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        auto atangentY = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        auto atangentZ = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        for (unsigned int i2 = 0; i2 < 3; i2++) {
            auto indexIntoIndices = triangleIndex * 3 + i2;
            auto vertexIndex = indices.at(indexIntoIndices);
            vertices.at(nFloatsPerVertex * vertexIndex + 8) = atangentX;
            vertices.at(nFloatsPerVertex * vertexIndex + 9) = atangentY;
            vertices.at(nFloatsPerVertex * vertexIndex + 10) = atangentZ;
        }
        
    }
    
    unsigned int meshId = LAST_MESH_ID; // (creating a mesh increments this)
    auto ptr = std::shared_ptr<Mesh>(new Mesh(vertices, indices, instanceColor, instanceTextureZ, expectedCount));
    LOADED_MESHES[meshId] = ptr;
    return ptr;
}

// unloads the mesh with the given meshId, freeing its memory.
// you CAN unload a mesh while objects are using it without any issues - a copy of that mesh is still on the gpu.
// you only need to call this function if you are (like for procedural terrain) dynamically loading new meshes
void Mesh::Unload(int meshId) {
    assert(LOADED_MESHES.count(meshId) != 0 && "Mesh::Unload() was given an invalid meshId.");
    LOADED_MESHES.erase(meshId);
}

Mesh::Mesh(const std::vector<GLfloat> &verts, const std::vector<GLuint> &indies, bool instanceColor, bool instanceTextureZ, unsigned int expectedCount):
meshId(LAST_MESH_ID++),
vertices(verts),
indices(indies),
instancedColor(instanceColor),
instancedTextureZ(instanceTextureZ),
instanceCount(expectedCount),
originalSize(),
vertexSize(sizeof(glm::vec3) + sizeof(glm::vec3) + sizeof(glm::vec3) + sizeof(glm::vec2) + ((!instancedColor) ? sizeof(glm::vec4) : 0) + ((!instancedTextureZ) ? sizeof(GLfloat) : 0)) // this is just sizeof(vertexPos) + sizeof(vertexNormals) + sizeof(textureXY) + sizeof(color if not instanced) + sizeof(textureZ if not instanced) 
{
    // std::cout << "Created mesh with id " << meshId << "\n";
}


