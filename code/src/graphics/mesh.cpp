#include "mesh.hpp"
#include "texture.hpp"
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "glm/gtc/type_ptr.hpp"
#include "gengine.hpp"
#include <list>
#include "../debug/log.hpp"
#include "gameobjects/render_component.hpp"
#include "physics/physics_mesh.hpp"

// TODO: this file takes too long to compile



std::shared_ptr<Mesh> Mesh::New(const MeshProvider& provider, bool dynamic) {
    // create/return actual mesh object
	unsigned int meshId = MeshGlobals::Get().LAST_MESH_ID; // (creating a mesh increments this)

	auto realParams = provider.meshParams;
	if (realParams.meshVertexFormat.has_value() == false) { realParams.meshVertexFormat.emplace(MeshVertexFormat::Default()); }
    auto product = provider.GetMesh();
    auto mesh = std::shared_ptr<Mesh>(new Mesh(product.first, product.second, realParams, dynamic));
	MeshGlobals::Get().LOADED_MESHES[meshId] = mesh;
	return mesh;
}

//void MeshGlobals::PurgeLoaded()
//{
//    LOADED_MESHES.clear();
//}

MeshGlobals& MeshGlobals::Get() {
    #ifdef IS_MODULE
    Assert(_MESH_GLOBALS != nullptr);
    return *_MESH_GLOBALS;
    #else
    static MeshGlobals globals;
    return globals;
    #endif
}

MeshGlobals::MeshGlobals() : MESHES_TO_PHYS_MESHES() {  }
MeshGlobals::~MeshGlobals() {  }

// helper thing for Mesh::FromFile() and TextMeshFromText() that will expand the vector so that it contains index if needed, then return vector.at(index)
template<typename T>
typename std::vector<T>::reference vectorAtExpanding(unsigned int index, std::vector<T>& vector) {
    if (vector.size() <= index) {
        vector.resize(index + 1, 6.66); // 6.66 is chosen so that uninitialized values are easy to see when debugging
    }
    return vector.at(index);
}

bool Mesh::IsValidForGameObject(unsigned int meshId) {
    return MeshGlobals::Get().LOADED_MESHES.count(meshId) && MeshGlobals::Get().LOADED_MESHES.at(meshId)->instancedVertexSize > 0;
}

std::shared_ptr<Mesh> Mesh::Square() {
    const static std::vector<GLfloat> squareVerts = {
        -0.5, -0.5, 0.0,   0.0, 0.0, 
         0.5, -0.5, 0.0,   1.0, 0.0,
         0.5,  0.5, 0.0,   1.0, 1.0,
        -0.5,  0.5, 0.0,   0.0, 1.0,
    };
    const static std::vector<GLuint> squareIndices = { 0, 1, 2, 0, 2, 3 };

    // IF IT DOESN"T WORK LOOK HERE, i think this static variable is okay
    static std::shared_ptr<Mesh> m = Mesh::New(RawMeshProvider(squareVerts, squareIndices, MeshCreateParams {.meshVertexFormat = MeshVertexFormat::DefaultGui(), .opacity = 1.0, .expectedCount = 1, .normalizeSize = false}), false);
    return m;
}





// engine calls this to get mesh from an object's meshId
std::shared_ptr<Mesh>& Mesh::Get(unsigned int meshId) {
    Assert(MeshGlobals::Get().LOADED_MESHES.count(meshId) != 0 && "Mesh::Get() was given an invalid meshId.");
    return MeshGlobals::Get().LOADED_MESHES[meshId];
}

// // Scales the vertex positions by the given 
// glm::vec3 NormalizeVertices(std::vector<GLfloat> & verts) {

// }

//std::shared_ptr<Mesh> Mesh::FromVertices(const std::vector<GLfloat>& verts, const std::vector<GLuint> &indies, const MeshCreateParams& params) {
//    unsigned int meshId = MeshGlobals::Get().LAST_MESH_ID; // (creating a mesh increments this)
//    auto ptr = std::shared_ptr<Mesh>(new Mesh(verts, indies, params));
//    MeshGlobals::Get().LOADED_MESHES[meshId] = ptr;
//    return ptr;
//}





// TODO: really bad right now
// std::shared_ptr<Mesh> Mesh::FromText(const std::string &text, const Texture &font, const MeshVertexFormat& vertexFormat, const bool isDynamic) {

//     Assert(vertexFormat.attributes.position.has_value() && vertexFormat.attributes.position->nFloats >= 2);
//     Assert(vertexFormat.attributes.textureUV.has_value() && vertexFormat.attributes.textureUV->nFloats >= 2);
//     Assert(font.usage == Texture::FontMap);
//     Assert(font.glyphs.has_value());

//     std::vector<GLfloat> vertices;
//     std::vector<GLuint> indices;

//     TextMeshFromText(text, font, vertexFormat, vertices, indices);

//     unsigned int i = LAST_MESH_ID;
//     auto ptr = std::shared_ptr<Mesh>(new Mesh(vertices, indices, isDynamic, vertexFormat, 1, false, true));
//     LOADED_MESHES[i] = ptr;
//     return ptr;
// }

//std::shared_ptr<Mesh> Mesh::Text(const MeshCreateParams& params) {
//    unsigned int i = MeshGlobals::Get().LAST_MESH_ID;
//    auto realParams = MeshCreateParams {.meshVertexFormat = params.meshVertexFormat, .expectedCount = 1, .normalizeSize = false, .dynamic = true};
//    auto ptr = std::shared_ptr<Mesh>(new Mesh(Mesh::Square()->meshVertices, Mesh::Square()->meshIndices, realParams, true));
//    MeshGlobals::Get().LOADED_MESHES[i] = ptr;
//    return ptr;
//}



// std::shared_ptr<Mesh> Mesh::FromFile(const std::string& path, const MeshCreateParams& params) {
//     // Load file
//     auto config = tinyobj::ObjReaderConfig();
//     config.triangulate = true;
//     config.vertex_color = !params.meshVertexFormat.attributes.color->instanced;
//     bool success = MeshGlobals::Get().OBJ_LOADER.ParseFromFile(path, config);
//     if (!success) {
//         std::printf("Mesh::FromFile failed to load %s because %s", path.c_str(), MeshGlobals::Get().OBJ_LOADER.Error().c_str());
//         abort();
//     }

//     auto nFloatsPerVertex = params.meshVertexFormat.GetNonInstancedVertexSize()/sizeof(GLfloat);

//     auto shape = MeshGlobals::Get().OBJ_LOADER.GetShapes().at(0);
//     //auto material = OBJ_LOADER.GetMaterials().at(0);
//     auto attrib = MeshGlobals::Get().OBJ_LOADER.GetAttrib();

//     std::vector<GLfloat> & positions = attrib.vertices;
//     std::vector<GLfloat> & texcoordsXY = attrib.texcoords;
//     std::vector<GLfloat> & normals = attrib.normals;
//     std::vector<GLfloat> & colors = attrib.colors;

//     // take all the seperate colors, positions, etc. and put them in a single interleaved vertices thing with one indices
//     std::unordered_map<std::tuple<GLuint, GLuint, GLuint>, GLuint, hash_tuple::hash<std::tuple<GLuint, GLuint, GLuint>>> objIndicesToGlIndices; // tuple is (posIndex, texXyIndex, normalIndex)
//     std::vector<GLuint> indices;
//     std::vector<GLfloat> vertices;



//     unsigned int currentVertex = 0;
//     for (auto & index : shape.mesh.indices) {
//         auto indexTuple = std::make_tuple(index.vertex_index, index.texcoord_index, index.normal_index);

//         // if we have this exact vertex already, just add another index for it, otherwise append a new vertex
//         if (objIndicesToGlIndices.count(indexTuple) == 0) {

//             objIndicesToGlIndices[indexTuple] = vertices.size()/nFloatsPerVertex;
//             indices.push_back(currentVertex);

//             // position
//             if (params.meshVertexFormat.attributes.position.has_value() && !params.meshVertexFormat.attributes.position->instanced) {
//                 for (unsigned int i = 0; i < params.meshVertexFormat.attributes.position->nFloats; i++) {
//                     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.attributes.position->offset/sizeof(GLfloat) + i, vertices) = (positions[index.vertex_index * 3 + i]);
//                 }
//             }

//             // uv
//             if (params.meshVertexFormat.attributes.textureUV.has_value() && !params.meshVertexFormat.attributes.textureUV->instanced) {
//                 for (unsigned int i = 0; i < params.meshVertexFormat.attributes.textureUV->nFloats; i++) {
//                     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.attributes.textureUV->offset/sizeof(GLfloat) + i, vertices) = (texcoordsXY[index.texcoord_index * 2 + i]);
//                 }
//             }
            
//             // normal
//             if (params.meshVertexFormat.attributes.normal.has_value() && !params.meshVertexFormat.attributes.normal->instanced) {
//                 for (unsigned int i = 0; i < params.meshVertexFormat.attributes.normal->nFloats; i++) {
//                     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.attributes.normal->offset/sizeof(GLfloat) + i, vertices) = (normals[index.normal_index * 3 + i]);
//                 }
//             }

//             // tangent
//             if (params.meshVertexFormat.attributes.tangent.has_value() && !params.meshVertexFormat.attributes.tangent->instanced) {
//                 for (unsigned int i = 0; i < params.meshVertexFormat.attributes.tangent->nFloats; i++) {
//                     // we just set these to 0 for now, they're calculating for real at a later step
//                     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.attributes.tangent->offset/sizeof(GLfloat) + i, vertices) = (0);
//                 }
//             }
            
//             // textureZ
//             if (params.meshVertexFormat.attributes.textureZ.has_value() && !params.meshVertexFormat.attributes.textureZ->instanced) {
//                 for (unsigned int i = 0; i < params.meshVertexFormat.attributes.textureZ->nFloats; i++) {
//                     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.attributes.textureZ->offset/sizeof(GLfloat) + i, vertices) = (params.textureZ);
//                 }
//             }

//             // color
//             if (params.meshVertexFormat.attributes.color.has_value() && !params.meshVertexFormat.attributes.color->instanced) {
//                 for (unsigned int i = 0; i < params.meshVertexFormat.attributes.color->nFloats; i++) {
//                     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.attributes.color->offset/sizeof(GLfloat) + i, vertices) = (i == 3 ? params.opacity : colors[index.vertex_index * 3 + i]);
//                 }
//             }

//             // model matrix, though i fear for your sanity if this isn't instanced
//             if (params.meshVertexFormat.attributes.modelMatrix.has_value() && !params.meshVertexFormat.attributes.modelMatrix->instanced) {
//                 for (unsigned int i = 0; i < params.meshVertexFormat.attributes.modelMatrix->nFloats; i++) {
//                     // we just set these to 0 for now, it's the user's job to set values to them
//                     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.attributes.modelMatrix->offset/sizeof(GLfloat) + i, vertices) = (0);
//                 }
//             }

//             // normal matrix, again why would you not instance this
//             if (params.meshVertexFormat.attributes.normalMatrix.has_value() && !params.meshVertexFormat.attributes.normalMatrix->instanced) {
//                 for (unsigned int i = 0; i < params.meshVertexFormat.attributes.normalMatrix->nFloats; i++) {
//                     // we just set these to 0 for now, it's the user's job to set values to them
//                     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.attributes.normalMatrix->offset/sizeof(GLfloat) + i, vertices) = (0);
//                 }
//             }

//             if (params.meshVertexFormat.attributes.arbitrary1.has_value() && !params.meshVertexFormat.attributes.arbitrary1->instanced) {
//                 for (unsigned int i = 0; i < params.meshVertexFormat.attributes.arbitrary1->nFloats; i++) {
//                     // we just set these to 0 for now, it's the user's job to set values to them
//                     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.attributes.arbitrary1->offset/sizeof(GLfloat) + i, vertices) = (0);
//                 }
//             }

//             if (params.meshVertexFormat.attributes.arbitrary2.has_value() && !params.meshVertexFormat.attributes.arbitrary2->instanced) {
//                 for (unsigned int i = 0; i < params.meshVertexFormat.attributes.arbitrary2->nFloats; i++) {
//                     // we just set these to 0 for now, it's the user's job to set values to them
//                     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.attributes.arbitrary2->offset/sizeof(GLfloat) + i, vertices) = (0);
//                 }
//             }
            
//             // vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.textureUV->offset/sizeof(GLfloat), vertices) = (texcoordsXY[index.texcoord_index * 2]); 
//             // vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.textureUV->offset/sizeof(GLfloat) + 1, vertices) = (texcoordsXY[index.texcoord_index * 2 + 1]);
            
//             // vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.normal->offset/sizeof(GLfloat), vertices) = (normals[index.normal_index * 3]);
//             // vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.normal->offset/sizeof(GLfloat) + 1, vertices) = (normals[index.normal_index * 3 + 1]);
//             // vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.normal->offset/sizeof(GLfloat) + 2, vertices) = (normals[index.normal_index * 3 + 2]);


//             // if (params.meshVertexFormat.color && !params.meshVertexFormat.color->instanced) {
//             //     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.color->offset/sizeof(GLfloat), vertices) = (colors[index.vertex_index * 3]);
//             //     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.color->offset/sizeof(GLfloat), vertices) = (colors[index.vertex_index * 3 + 1]);
//             //     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.color->offset/sizeof(GLfloat), vertices) = (colors[index.vertex_index * 3 + 2]);
//             //     vectorAtExpanding(currentVertex * nFloatsPerVertex + params.meshVertexFormat.color->offset/sizeof(GLfloat), vertices) = (meshTransparency);
//             // }

//             currentVertex += 1;
//         }
//         else {
//             indices.push_back(objIndicesToGlIndices[indexTuple]);
//         }
//     }

//     // calculate tangent vectors
//     if (params.meshVertexFormat.attributes.tangent.has_value() && params.meshVertexFormat.attributes.tangent->instanced == false) {

//         std::vector<GLfloat> tangents;
        
//         // to calculate tangent vectors, we must put a few restrictions on the meshVertexFormat.
//         Assert(params.meshVertexFormat.attributes.position.has_value() && params.meshVertexFormat.attributes.position->nFloats >= 3 && params.meshVertexFormat.attributes.position->instanced == false);
//         Assert(params.meshVertexFormat.attributes.textureUV.has_value() && params.meshVertexFormat.attributes.textureUV->nFloats >= 2 && params.meshVertexFormat.attributes.textureUV->instanced == false);
//         Assert(params.meshVertexFormat.attributes.normal.has_value() && params.meshVertexFormat.attributes.normal->nFloats >= 3 && params.meshVertexFormat.attributes.normal->instanced == false);
//         Assert(params.meshVertexFormat.attributes.tangent->nFloats >= 3);

//         for (unsigned int triangleIndex = 0; triangleIndex < indices.size()/3; triangleIndex++) {
//             glm::vec3 points[3];
//             glm::vec2 texCoords[3];
//             glm::vec3 l_normals[3]; // l_ to avoid shadowing
//             for (unsigned int j = 0; j < 3; j++) {
//                 auto indexIntoIndices = triangleIndex * 3 + j;
//                 //std::cout << " i = " << indexIntoIndices << "nfloats = " << nFloatsPerVertex << " actual index = " << indices.at(indexIntoIndices) <<  " \n";
//                 auto vertexIndex = indices.at(indexIntoIndices);
//                 //std::cout << "thing in indices  was " << vertexIndex << " \n"; 
//                 points[j] = glm::make_vec3(&vertices.at(nFloatsPerVertex * vertexIndex + params.meshVertexFormat.attributes.position->offset/sizeof(GLfloat)));
//                 texCoords[j] = glm::make_vec2(&vertices.at(nFloatsPerVertex * vertexIndex + params.meshVertexFormat.attributes.textureUV->offset/sizeof(GLfloat)));
//                 l_normals[j] = glm::make_vec3(&vertices.at(nFloatsPerVertex * vertexIndex + params.meshVertexFormat.attributes.normal->offset/sizeof(GLfloat)));
//             }
            
//             glm::vec3 edge1 = points[1] - points[0];
//             glm::vec3 edge2 = points[2] - points[0];
//             glm::vec2 deltaUV1 = texCoords[1] - texCoords[0];
//             glm::vec2 deltaUV2 = texCoords[2] - texCoords[0]; 
    
//             float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);


//             auto atangentX = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
//             auto atangentY = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
//             auto atangentZ = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

//             for (unsigned int i2 = 0; i2 < 3; i2++) {
//                 auto indexIntoIndices = triangleIndex * 3 + i2;
//                 auto vertexIndex = indices.at(indexIntoIndices);
//                 vertices.at(nFloatsPerVertex * vertexIndex + params.meshVertexFormat.attributes.tangent->offset/sizeof(GLfloat)) = atangentX;
//                 vertices.at(nFloatsPerVertex * vertexIndex + params.meshVertexFormat.attributes.tangent->offset/sizeof(GLfloat) + 1) = atangentY;
//                 vertices.at(nFloatsPerVertex * vertexIndex + params.meshVertexFormat.attributes.tangent->offset/sizeof(GLfloat) + 2) = atangentZ;
//             }
            
//         }
//     }
    
//     unsigned int meshId = MeshGlobals::Get().LAST_MESH_ID; // (creating a mesh increments this)
//     auto ptr = std::shared_ptr<Mesh>(new Mesh(vertices, indices, params));
//     MeshGlobals::Get().LOADED_MESHES[meshId] = ptr;
//     return ptr;
// }


void Mesh::Unload(int meshId) {
    int count = MeshGlobals::Get().LOADED_MESHES.count(meshId);
    Assert(count != 0 && "Mesh::Unload() was given an invalid meshId.");
    MeshGlobals::Get().LOADED_MESHES.erase(meshId);
}

// unsigned int MaxBonesFromAnims(std::optional<std::vector<Animation>> anims) {
//     if (!anims.has_value()) {return 0;}
//     unsigned int greatest = 0;
//     for (auto & anim: anims.value()) {
//         if (meshBones.size() > greatest) {
//             greatest = meshBones.size();
//         } 
//     }
//     return greatest;
// }

Mesh::~Mesh() {
    /*if (GraphicsEngine::Get().dynamicMeshLocations.count(meshId)) {
        GraphicsEngine::Get().dynamicMeshLocations.erase(meshId);
    }*/
    DebugLogInfo("Deleting mesh with id ", meshId);
}

Mesh::Mesh(const std::vector<GLfloat> &verts, const std::vector<GLuint> &indies, const MeshCreateParams& params, bool dynamic, bool fromText, std::optional<std::vector<Bone>> bonez, std::optional<std::vector<Animation>> anims, unsigned int rootBoneIndex):
dynamic(dynamic),
meshId(MeshGlobals::Get().LAST_MESH_ID++),
instanceCount(params.expectedCount),
nonInstancedVertexSize(params.meshVertexFormat.value().GetNonInstancedVertexSize()),
instancedVertexSize(params.meshVertexFormat.value().GetInstancedVertexSize()),
vertexFormat(params.meshVertexFormat.value()),
wasCreatedFromText(fromText),
meshVertices(verts),
meshIndices(indies),
meshBones(bonez),
meshAnimations(anims),
rootBoneId(rootBoneIndex),
originalSize(1)
{    
    DebugLogInfo("Generated mesh with id ", meshId);

    //DebugLogInfo("New mesh with id ", meshId);
    //Assert(meshVertices.size() > 0);
    //Assert(meshIndices.size() > 0);

    if (params.normalizeSize) {
        NormalizePositions();
    }

    if (meshAnimations) {
        Assert(meshBones->size() <= vertexFormat.maxBones);
        // Assert(meshAnimations->size() > 0);
        // TODO: ???
        // float nBones = meshAnimations->at(0).bones.size();
        // for (auto & anim: *meshAnimations) {
        //     if (anim.bones.size() != nBones) { // TODO: am i right here?????
        //         DebugLogError("The animation ", anim.name, " has ", anim.bones.size(), " bones, but at least one other animation on this mesh has ", nBones, " bones. All animations on any given mesh must have the same number of bones.");
        //     } 
        // }
    }
    
}

void Mesh::NormalizePositions() {
    Assert(vertexFormat.attributes.position.has_value() && !vertexFormat.attributes.position->instanced && (vertexFormat.attributes.position->nFloats == 2 || vertexFormat.attributes.position->nFloats == 3));

    // find mesh extents
    float minX = 0, maxX = 0, minY = 0, maxY = 0, minZ = 0, maxZ = 0;
    for (unsigned int i = 0; i < vertices.size(); i++) {
        const float & v = vertices[i];
        unsigned int remainder = (i % (nonInstancedVertexSize/sizeof(GLfloat))) - (vertexFormat.attributes.position->offset/sizeof(GLfloat));
        
        if (remainder == 0) {
            minX = std::min(minX, v);
            maxX = std::max(maxX, v);
        }
        else if (remainder == 1) {
            minY = std::min(minY, v);
            maxY = std::max(maxY, v);
        }
        else if (remainder == 2 && vertexFormat.attributes.position->nFloats == 3) {
            minZ = std::min(minZ, v);
            maxZ = std::max(maxZ, v);
        }
    }

    // scale mesh by extents
    for (unsigned int i = 0; i < vertices.size(); i++) {
        float & v = meshVertices[i];
        unsigned int remainder = (i % (nonInstancedVertexSize/sizeof(GLfloat))) - (vertexFormat.attributes.position->offset/sizeof(GLfloat));
        if (remainder == 0) {
            v = 1.0f*(v-minX)/(maxX-minX) - 0.5f; // i don't really know how this bit works i got it from stack overflow and modified it
        }
        else if (remainder == 1) {
            v = 1.0f*(v-minY)/(maxY-minY) - 0.5f; // i don't really know how this bit works i got it from stack overflow and modified it
        }
        else if (remainder == 2 && vertexFormat.attributes.position->nFloats == 3) {
            v = 1.0f*(v-minZ)/(maxZ-minZ) - 0.5f; // i don't really know how this bit works i got it from stack overflow and modified it
        }
    }

    originalSize = {maxX - minX, maxY - minY, maxZ - minZ};
}

std::pair<std::vector<GLfloat>&, std::vector<GLuint>&> Mesh::StartModifying() {
    Assert(dynamic == true);
    return {meshVertices, meshIndices};
}

void Mesh::StopModifying(bool normalizeSize) {
    Assert(dynamic == true);
    //Assert(meshVertices.size() > 0);
    //Assert(meshIndices.size() > 0);

    if (normalizeSize) {
        NormalizePositions();
    }

    for (PhysicsMesh* m : physicsUsers) {
        m->RefreshMesh();
    }

    if (GraphicsEngine::Get().dynamicMeshLocations.contains(meshId)) { //  this could be legitimately not the case if the mesh just isn't in use
        DebugLogInfo("Completing modification for ", meshId, " count ", indices.size());
        auto [meshpoolId, currentMeshSlot] = GraphicsEngine::Get().dynamicMeshLocations.at(meshId);
        Meshpool& pool = *GraphicsEngine::Get().meshpools.at(meshpoolId);
        DebugLogInfo("Current slot byte capacity is ", pow(2, (int)pool.meshSlotContents.at(pool.meshUsers.at(meshId)).sizeClass) * vertexFormat.GetNonInstancedVertexSize(), " we need capacity of ", std::max(indices.size() * sizeof(GLuint), vertexFormat.GetNonInstancedVertexSize() * vertices.size()));


        if // if the meshpool slot the mesh was in can still hold the mesh with its new size...
            (pow(2, (int)pool.meshSlotContents.at(pool.meshUsers.at(meshId)).sizeClass) * vertexFormat.GetNonInstancedVertexSize()
                >
                std::max(indices.size() * sizeof(GLuint), vertexFormat.GetNonInstancedVertexSize() * vertices.size()))
        { 
            DebugLogInfo("Slot still fits, resolving draw buffers.");
            // then just update the vertices and draw command and we're done
            // update mesh
            pool.meshUpdates.emplace_back(Meshpool::MeshUpdate{
                .updatesLeft = MESH_BUFFERING_FACTOR,
                .mesh = shared_from_this(),
                .meshIndex = pool.meshUsers.at(meshId),
            });

            // update draw command
            for (auto& commandBuffer : pool.drawCommands) {
                if (!commandBuffer.has_value()) continue;
                if (!commandBuffer->dynamicMeshCommandLocations.contains(meshId)) continue;

                auto commandIndices = commandBuffer->dynamicMeshCommandLocations.at(meshId);
                DebugLogInfo("UPdating ", commandIndices.size());
                for (auto& i : commandIndices) {
                    auto& ogCommand = commandBuffer->clientCommands[i];
                    auto newCommand = IndirectDrawCommand{
                            .count = indices.size(),
                            .instanceCount = ogCommand.instanceCount,
                            .firstIndex = ogCommand.firstIndex,
                            .baseVertex = ogCommand.baseVertex,
                            .baseInstance = ogCommand.baseInstance,
                    };
                    commandBuffer->commandUpdates.emplace_back(IndirectDrawCommandUpdate{
                        .updatesLeft = INSTANCED_VERTEX_BUFFERING_FACTOR,
                        .command = newCommand,
                        .commandSlot = i   
                    });
                    commandBuffer->clientCommands[i] = newCommand;
                }
            }
        }
        else { // we have to move the mesh to a new slot, which means removing each render component and readding it to the meshpool. 
            // todo: could potentially do some crazy optimization here by manually wiping the mesh from the pool then directly setting pool ids for new pool?
            // low priority in any case
            // TODO: could making component no longer in a meshpool could break stuff that calls methods of rendercomponent before it's readded?

            auto componentsToUpdate = GraphicsEngine::Get().dynamicMeshUsers.at(meshId); // deliberate vector copy. Calling RemoveObject() will modify the vector otherwise which is undesirable
            for (auto& renderComponent : componentsToUpdate) {
                GraphicsEngine::Get().RemoveObject(renderComponent);
                renderComponent->drawHandle.drawBufferIndex = -1;
                renderComponent->drawHandle.instanceSlot = -1;
                renderComponent->drawHandle.meshIndex = -1;
                GraphicsEngine::Get().AddObject(renderComponent->shaderProgramId, renderComponent->materialId, meshId, renderComponent);
            }
        }
    }
    
}

void Mesh::Remesh(const MeshProvider& provider, bool normalizeSize)
{
    auto [v, i] = StartModifying();
    auto [newV, newI] = provider.GetMesh();
    v = newV;
    i = newI;
    StopModifying(normalizeSize);
}
