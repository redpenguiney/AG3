#pragma once
#include <deque>
#include <tuple>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include "buffered_buffer.hpp"
#include "mesh.hpp"
#include "indirect_draw_command.cpp"

const unsigned int INSTANCED_VERTEX_BUFFERING_FACTOR = 3;

// TODO: INDBO shouldn't be persistent, and arguably neither should the vertices/indices.
// TODO: INDBO should just be written to directly instead of writing to drawCommands and then doing memcpy.

// Contains an arbitrary number of arbitary meshes and is used to render them very quickly.
class Meshpool {
    public:
    Meshpool(std::shared_ptr<Mesh>& mesh);
    Meshpool(const Meshpool&) = delete; // try to copy construct and i will end you

    std::vector<std::pair<unsigned int, unsigned int>> AddObject(const unsigned int meshId, unsigned int count);
    void RemoveObject(const unsigned int slot, const unsigned int instanceId); 
    void SetModelMatrix(const unsigned int slot, const unsigned int instanceId, const glm::mat4x4 model);
    void SetColor(const unsigned int slot, const unsigned int instanceId, const glm::vec4 rgba);
    void SetTextureZ(const unsigned int slot, const unsigned int instanceId, const float textureZ);
    //std::tuple<GLfloat*, const unsigned int> ModifyVertices(const unsigned int meshId);
    void Draw();
    void Update();

    int ScoreMeshFit(const unsigned int verticesNBytes, const unsigned int indicesNBytes, const bool shouldInstanceColor, const bool shouldInstanceTextureZ);

    private:
    const unsigned int instanceSize; // Each gameobject has one instance containing its data (at minimum a model matrix)
    const unsigned int vertexSize; // the size of a single vertex. (not to be confused with meshVertexSize, the maximum combined size of a mesh's vertices allowed by the pool)
    const unsigned int meshVerticesSize; // The vertex data of meshes inside the pool can be smaller but no bigger than this (if they're way smaller they should still go in a new mesh pool)
    const unsigned int meshIndicesSize; // same as meshVertexSize but for the indices of a mesh
    const bool instanceColor;
    const bool instanceTextureZ;

    const unsigned int baseMeshCapacity; // everytime the mesh pool expands its non-instanced vertex buffer, it will add room for this many meshes (or a multiple of this number if more than baseMeshCapacity meshes were added at once)
    const unsigned int baseInstanceCapacity; // same as baseMeshCapacity but for the instanced vertex buffer
    unsigned int meshCapacity; // how many different meshes the pool can store
    unsigned int instanceCapacity; // how many instances the pool can store (every gameobject being rendered needs exactly one instance with position and what not, but they can share meshes)

    unsigned int drawCount;

    GLuint vao; // tells opengl how vertices are formatted

    BufferedBuffer vertexBuffer; // holds per-vertex data (like normals)
    BufferedBuffer instancedVertexBuffer; // holds per-instance/per-object data (like model matrix)
    BufferedBuffer indexBuffer; // holds mesh indices
    BufferedBuffer indirectDrawBuffer; // stores rendering commands, used for an optimization called indirect drawing

    std::unordered_map<unsigned int, std::deque<unsigned int>> availableMeshSlots; // key is mesh instanceCapacity (or 0 if slot does not yet have instanceCapacity forced into it), value is deque of available slots
    //std::deque<unsigned int> availableInstancedSlots;
    std::unordered_map<unsigned int, std::vector<unsigned int>> slotContents; // key is meshId, value is a vector of indices/slots in the vertexBuffer containing this mesh (0 for first mesh, 1 for second, etc.)
                                                                              // needed for instancing

    std::unordered_map<unsigned int, unsigned int> slotInstanceReservedCounts; // key is slot, value is number of instances reserved by that slot 

    std::unordered_map<unsigned int, std::unordered_set<unsigned int>> slotInstanceSpaces; // key is slot, value is set of instances that aren't actually being drawn as their model matrix is set to all 0s 
    // neccesary for Meshpool::RemoveObject(); unordered_set instead of vector because RemoveObject needs to quickly determine if certain instances are in here

    std::unordered_map<unsigned int, unsigned int> slotToInstanceLocations; // corresponds slots in vertexBuffer with the instance slot of the first object using that meshId
    std::vector<IndirectDrawCommand> drawCommands; // for indirect drawing

    inline static const unsigned int TARGET_VBO_SIZE = pow(2, 24); // we want vbo base size to be around this size, so we set meshCapacity based off this value/meshVertexSize
                                                                        // 16MB might be too low if we're adding many different large meshes

    inline static const GLuint POS_ATTRIBUTE = 0;
    inline static const GLuint COLOR_ATTRIBUTE = 1; // color is rgba
    inline static const GLuint TEXTURE_XY_ATTRIBUTE = 2;
    inline static const GLuint TEXTURE_Z_ATTRIBUTE = 3;
    inline static const GLuint NORMAL_ATTRIBUTE = 4; // lighting needs to know normals
    inline static const GLuint MODEL_MATRIX_ATTRIBUTE = 5; // one 4x4 model matrix per thing being drawn, multiplying vertex positions by this puts them in the right position via rotating, scaling, and translating
    inline static const GLuint NORMAL_MATRIX_ATTRIBUTE = 9; // normal matrix is like model matrix, but is 3x3 and for normals since it would be bad if a normal got scaled/translated 

    void ExpandNonInstanced();
    void ExpandInstanced(GLuint multiplier);

    void FillSlot(const unsigned int meshId, const unsigned int slot, const unsigned int instanceCount);
    void UpdateIndirectDrawBuffer(const unsigned int slot);
};