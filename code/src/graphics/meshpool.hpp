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
// TODO: MASSIVE MEMORY OPTIMIZATION WHEN SAME OBJECT IS USED WITH MULTIPLE DIFFERENT SHADERS/MATERIALS: just one meshpool per object size, different indbos for different materials/shaders

// Contains an arbitrary number of arbitary meshes and is used to render them very quickly.
class Meshpool {
    public:

    // constructor takes mesh format reference to set variables, doesn't actually add the given mesh or anything
    // numVertices is number of actual vertices, not size of vertices vector in the mesh
    Meshpool(const MeshVertexFormat& meshVertexFormat, unsigned int numVertices);

    Meshpool(const Meshpool&) = delete; // try to copy construct and i will end you

    // Adds count identical meshes to pool, and returns a vector of pairs of (slot, instanceOffset) used to access the object.
    std::vector<std::pair<unsigned int, unsigned int>> AddObject(const unsigned int meshId, unsigned int count);

    // Frees the given object from the meshpool, so something else can use that space.
    void RemoveObject(const unsigned int slot, const unsigned int instanceId); 

    // Makes the given instance use the given normal matrix.
    // Will abort if mesh uses per-vertex normal matrix instead of per-instance normal matrix. (though who would do that???)
    void SetNormalMatrix(const unsigned int slot, const unsigned int instanceId, const glm::mat3x3& normal);

    // Makes the given instance use the given model matrix.
    // Will abort if mesh uses per-vertex model matrix instead of per-instance model matrix. (though who would do that???)
    void SetModelMatrix(const unsigned int slot, const unsigned int instanceId, const glm::mat4x4& model);

    // Sets the bone transforms. Do not call if the meshpool vertex format does not support animation.
    void SetBoneState(const unsigned int slot, const unsigned int instanceId, unsigned int nBones, glm::mat4x4* offsets);

    // // Makes the given instance the given textureZ. Internally just makes the equivalent call to SetInstancedVertexAttribute().
    // // Will abort if mesh uses per-vertex textureZ instead of per-instance textureZ.
    // void SetTextureZ(const unsigned int slot, const unsigned int instanceId, const float textureZ);

    // // Makes the given instance the given color. Internally just makes the equivalent call to SetInstancedVertexAttribute().
    // // Will abort if mesh uses per-vertex color instead of per-instance color.
    // void SetColor(const unsigned int slot, const unsigned int instanceId, const glm::vec4& rgba);

    // Set the given instanced vertex attribute of the given instance to the given value.
    // Will abort if nFloats does not match the mesh's vertex format or if the vertex attribute is not instanced.
    template<unsigned int nFloats>
    void SetInstancedVertexAttribute(const unsigned int slot, const unsigned int instanceId, const unsigned int attributeName, const glm::vec<nFloats, float>& value);

    //std::tuple<GLfloat*, const unsigned int> ModifyVertices(const unsigned int meshId);
    void Draw();

    // needed for BufferedBuffer's double/triple buffering, call every frame AFTER writing vertex/instance data and BEFORE calling Draw().
    void Commit();
    // needed for BufferedBuffer's double/triple buffering, call every frame BEFORE writing vertex/instance data and AFTER calling Draw(). 
    // Might yield if GPU isn't ready for us to write the data, so call at the last possible second.
    void FlipBuffers();

    // We want meshes to fit snugly in the slots of their meshpool.
    // Returns -1 if mesh is too big/incompatible format w/ meshpool, otherwise lower number = better fit for meshpool.
    // The engine will put meshes in the best fitting pool.
    int ScoreMeshFit(const unsigned int verticesNBytes, const unsigned int indicesNBytes, const MeshVertexFormat& meshVertexFormat);

    private:
    friend class Mesh; // for dynamic mesh support, idc about modularity

    const unsigned int instancedVertexSize; // Each gameobject has one instance containing its data (at minimum a model matrix)
    const unsigned int nonInstancedVertexSize; // the size of a single vertex in bytes. (not to be confused with meshVertexSize, the maximum combined size of a mesh's vertices allowed by the pool)
    const unsigned int meshVerticesSize; // The vertex data of meshes inside the pool can be smaller but no bigger than this ( in bytes) (if they're way smaller they should still go in a new mesh pool)
    const unsigned int meshIndicesSize; // same as meshVertexSize but for the indices of a mesh, again in bytes
    const MeshVertexFormat vertexFormat; // only meshes that store their vertices in the same way can go in the same meshpool, so we store it here to check

    const unsigned int baseMeshCapacity; // everytime the mesh pool expands its non-instanced vertex buffer, it will add room for this many meshes (or a multiple of this number if more than baseMeshCapacity meshes were added at once)
    const unsigned int baseInstanceCapacity; // same as baseMeshCapacity but for the instanced vertex buffer
    unsigned int meshCapacity; // how many different meshes the pool can store
    unsigned int instanceCapacity; // how many instances the pool can store (every gameobject being rendered needs exactly one instance with position and what not, but they can share meshes)

    unsigned int drawCount;

    GLuint vao; // tells opengl how vertices are formatted

    // TODO: all these unordered maps are probably overkill, vectors would probably be faster.

    BufferedBuffer vertexBuffer; // holds per-vertex data (like normals)
    BufferedBuffer instancedVertexBuffer; // holds per-instance/per-object data (like model matrix)
    BufferedBuffer indexBuffer; // holds mesh indices
    BufferedBuffer indirectDrawBuffer; // stores rendering commands, used for an optimization called indirect drawing
    // Bones and instances have 1-1 correspondence. The nth instance has the nth boneOffset and entry in the bonebuffer
    std::optional<BufferedBuffer> boneBuffer; // if the shader/mesh combo supports animations, this will store the bone transform matrices (and the count)
    std::optional<BufferedBuffer> boneOffsetBuffer; // if the shader/mesh combo supports animations, stores offsets into the bone buffer for each object


    std::unordered_map<unsigned int, std::deque<unsigned int>> availableMeshSlots; // key is mesh instanceCapacity (or 0 if slot does not yet have instanceCapacity forced into it), value is deque of available slots
    //std::deque<unsigned int> availableInstancedSlots;
    std::unordered_map<unsigned int, std::vector<unsigned int>> slotContents; // key is meshId, value is a vector of indices/slots in the vertexBuffer containing this mesh (0 for first mesh, 1 for second, etc.)
                                                                              // needed for instancing

    std::unordered_map<unsigned int, unsigned int> slotInstanceReservedCounts; // key is slot, value is number of instances reserved by that slot 

    std::unordered_map<unsigned int, std::unordered_set<unsigned int>> slotInstanceSpaces; // key is slot, value is set of instances that aren't actually being drawn as their model matrix is set to all 0s (TODO: WAIT WHAT WHY DO WE DO IT LIKE THAT) 
    // neccesary for Meshpool::RemoveObject(); unordered_set instead of vector because RemoveObject needs to quickly determine if certain instances are in here
    
    std::unordered_map<unsigned int, unsigned int> slotToInstanceLocations; // corresponds slots in vertexBuffer with the instance slot of the first object using that meshId
    std::vector<IndirectDrawCommand> drawCommands; // for indirect drawing

    inline static const unsigned int TARGET_VBO_SIZE = (unsigned int)pow(2, 24); // we want vbo base size to be around this size, so we set meshCapacity based off this value/meshVertexSize
                                                                        // 16MB might be too low if we're adding many different large meshes

    // inline static const unsigned int BONE_BUFFER_EXPANSION_SIZE = pow(2, 10) * sizeof(glm::mat4x4); 
    // inline static const unsigned int BONE_OFFSET_BUFFER_EXPANSION_SIZE = pow(2, 10) * sizeof(GLuint);
    // (0 is used by lights)
    inline static const unsigned int BONE_BUFFER_BINDING = 1;
    inline static const unsigned int BONE_OFFSET_BUFFER_BINDING = 2;

    
    // expands non-instanced buffers to accomodate baseMeshCapacity more unique meshes.
    // also makes the vao
    void ExpandNonInstanced();

    // VAO MUST BE GENERATED BEFORE THIS FUNCTION IS CALLED.
    // expands instance buffer to accomodate multiplier * baseInstanceCapacity more instances, or creates the instance buffer if needed.
    // this one needs a multiplier argument because if we add like 20000 instances and the baseInstanceCapacity is 5000, there wouldn't be enough room and it would segfault
    // also expands animation/bone buffers.
    void ExpandInstanced(GLuint multiplier);
    // void ExpandAnimationBuffers(GLuint multiplier);

    // if modifying == true, then the slot already contains this mesh and it's just resetting the vertex data within (solely for modifying dynamic meshes).
    // (instanceCount argument is meaningless when modifying == true.)
    // if modifying == false, it's adding a whole new mesh to a slot that was previously either empty or holding a different mesh before. TODO: possibly won't work idk
    // Fills the given slot with the given mesh's vertices and indices.
    void FillSlot(const unsigned int meshId, const unsigned int slot, const unsigned int instanceCount, bool modifiying);

    void UpdateIndirectDrawBuffer(const unsigned int slot);
};