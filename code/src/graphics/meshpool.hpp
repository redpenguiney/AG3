#pragma once

#include <optional>
#include <array>
#include <vector>
#include <memory>

#include "buffered_buffer.hpp"
#include "mesh_provider.hpp"
#include "indirect_draw_command.hpp"

#include <glm/mat4x4.hpp>
#include <glm/mat3x3.hpp>
#include "utility/uint.hpp"

const unsigned int INSTANCED_VERTEX_BUFFERING_FACTOR = 3; /// TODO: based on mesh params?
const unsigned int MESH_BUFFERING_FACTOR = 1; /// TODO; meshpool supports >1 but GE doesn't

class Mesh;
class Material;
class ShaderProgram;

// Contains an arbitrary number of arbitary meshes (of the same MeshVertexFormat) and is used to render them very quickly.
class Meshpool {
public:
    
    // Each object that gets rendered needs a DrawHandle to describe where its data is stored in its meshpool.
    struct DrawHandle {
        // An index, in terms of vertexSize, to where the mesh vertices are stored in the vertices buffer, and in terms of indexSize to where the mesh indices are stored in the index buffer. 
        // So if meshIndex was 3000, the first byte of the mesh would be at vertices.Data() + (3000 * vertexSize)
        int meshIndex;

        // An index (in terms of instanceSize) to where the object's instance data is stored in the instances buffer.
        int instanceSlot;

        // Index into drawCommands; which INDBO the object's draw command is stored in.
        int drawBufferIndex;
    };

    // Only meshes with this format can be stored in this meshpool.
    const MeshVertexFormat format;

    Meshpool(const MeshVertexFormat& meshVertexFormat);

    Meshpool(const Meshpool&) = delete; // try to copy construct and i will end you

    ~Meshpool();

    // Adds count identical objects to the pool with the given mesh, and returns a DrawHandle for each object.
    // Material should be NULLPTR if the object has no material.
    std::vector<DrawHandle> AddObject(const std::shared_ptr<Mesh>&, const std::shared_ptr<Material>&, const std::shared_ptr<ShaderProgram>&, CheckedUint count);

    // Frees the given object from the meshpool, so something else can use that space.
    void RemoveObject(const DrawHandle& handle);

    // Makes the given instance use the given normal matrix.
    // Will abort if mesh uses per-vertex normal matrix instead of per-instance normal matrix. (though who would do that???)
    //void SetNormalMatrix(const DrawHandle& handle, const glm::mat3x3& normal);

    // Makes the given instance use the given model matrix.
    // Will abort if mesh uses per-vertex model matrix instead of per-instance model matrix. (though who would do that???)
    //void SetModelMatrix(const DrawHandle& handle, const glm::mat4x4& model);

    // Sets the bone transforms. Do not call if the meshpool vertex format does not support animation.
    void SetBoneState(const DrawHandle& handle, CheckedUint nBones, glm::mat4x4* offsets);

    // Set the given instanced vertex attribute of the given instance to the given value.
    // Will abort if AttributeType does not match the mesh's vertex format or if the vertex attribute is not instanced.
    template<typename AttributeType>
    void SetInstancedVertexAttribute(const DrawHandle& handle, const CheckedUint attributeName, const AttributeType& value);

    // idk what to put here, you probably know what this does
    // prePostProc is true if this is being drawn BEFORE post processing runs
    void Draw(bool prePostProc);

    // needed for BufferedBuffer's double/triple buffering, call every frame AFTER writing vertex/instance data and BEFORE calling Draw().
    void Commit();

    // needed for BufferedBuffer's double/triple buffering, call every frame BEFORE writing vertex/instance data and AFTER calling Draw(). 
    // Might yield if GPU isn't ready for us to write the data, so call at the last possible second.
    void FlipBuffers();


private:
    inline static const CheckedUint BONE_BUFFER_BINDING = 2;
    inline static const CheckedUint BONE_OFFSET_BUFFER_BINDING = 3;

    struct MeshUpdate {
        CheckedUint updatesLeft;
        std::shared_ptr<Mesh> mesh;

        // vertex index/slot
        CheckedUint meshIndex;
    };

    // needed by RemoveObject()
    struct CommandLocation {

        // RemoveObject() knows which indirect draw buffer this index is referring to from the DrawHandle
        CheckedUint drawCommandIndex;

        // Here, command.baseInstance and command.baseVertex presume no multiple buffering. Meshpool::Commit() corrects that.
        //IndirectDrawCommand command;
    };

    struct MeshSlotUsageInfo {
        // id of the mesh inside this slot
        CheckedUint meshId;
        CheckedUint sizeClass; // index of the vector in availableMeshSlots that this goes in when its freed
        CheckedUint nUsers; // num draw commands using this mesh slot
    };

    class DrawCommandBuffer {
    public:
        DrawCommandBuffer(const std::shared_ptr<ShaderProgram>&, const std::shared_ptr<Material>&, BufferedBuffer&&);

        // may not be nullptr
        std::shared_ptr<ShaderProgram> shader;

        // may be nullptr
        std::shared_ptr<Material> material;

        BufferedBuffer buffer;

        // number of commands in buffer
        //CheckedUint drawCount = 0;

        CheckedUint currentDrawCommandCapacity = 0;

        // unlike the other two available<x>slots, this one does hold every slot that isn't taken and if this is empty, then you have to expand.
    // Sorted from greatest to least.
        std::vector<CheckedUint> availableDrawCommandSlots = {};

        std::vector<IndirectDrawCommandUpdate> commandUpdates = {};

        // all 0s for empty/available command slots
        std::vector<IndirectDrawCommand> clientCommands = {}; // equivelent contents to drawCommands, but we can't read from that because its a GPU buffer

        CheckedUint GetNewDrawCommandSlot();

        int GetDrawCount();

        // Doubles currentDrawCommandCapacity.
        void ExpandDrawCommandCapacity();

        // bc bufferedbuffer can only be moved
        DrawCommandBuffer(DrawCommandBuffer&&) noexcept;
        DrawCommandBuffer& operator=(DrawCommandBuffer&& old) noexcept;    // move assignment operator

        DrawCommandBuffer(const DrawCommandBuffer&) = delete;
        DrawCommandBuffer& operator=(const DrawCommandBuffer&) = delete;
    };

    // the size of a single instance for a single object in bytes. Equal to the InstancedSize() of the vertex format.
    const CheckedUint instanceSize;

    // the size of a single vertex for a single mesh in bytes. Equal to the NonInstancedSize() of the vertex format.
    const CheckedUint vertexSize;

    constexpr static unsigned int indexSize = sizeof(GLuint);

    CheckedUint currentVertexCapacity;
    CheckedUint currentInstanceCapacity;



    // if you want to allocate memory for a mesh more than vertexSize * 2^(n-1) bytes but less than vertexSize * 2^n bytes, the nth vector in this array has room for that.
    // When a mesh is removed from the pool, an index to its memory goes here.
    // The CheckedUints in each vector are vertex indices (same unit as DrawHandle.meshIndex)
    std::array<std::vector<CheckedUint>, 32> availableMeshSlots;

    // key is mesh slot. Needed for RemoveObject() to know where to put freed mesh 
    // TODO: is unordered_map necceasry?
    std::unordered_map<CheckedUint, MeshSlotUsageInfo> meshSlotContents;

    // Key is meshId, value is mesh slot. Needed for AddObject() to know when they already have the wanted mesh.
    // TODO: again, ditch unordered_map?
    std::unordered_map<CheckedUint, CheckedUint> meshUsers;

    // if availableMeshSlots is empty, this is the mesh index of the first available part of the vertices buffer
    CheckedUint meshIndexEnd;

    // For each mesh slot, describes meshId.
    //std::vector<SlotUsageInfo> meshSlotContents;

    std::vector<CheckedUint> availableInstanceSlots;
    CheckedUint instanceEnd;


    std::vector<MeshUpdate> meshUpdates;

    // key is instance slot
    std::vector<CommandLocation> instanceSlotsToCommands;



    // the VAO basically tells openGL how our vertices are structured
    unsigned int vaoId;

    // stores vertices for all the pool's meshes.
    BufferedBuffer vertices;

    // stores instances for each object being drawn.
    BufferedBuffer instances;

    // stores indices for all the pool's indices
    BufferedBuffer indices;

    // each buffer stores indirect draw commands, which basically tell the GPU which vertices/instances to draw.
    std::vector <std::optional< DrawCommandBuffer >> drawCommands;
    std::vector<CheckedUint> availableDrawCommandBufferIndices;

    // if the shader/mesh combo supports animations, stores the bone transform matrices (and the number of them)
    std::optional<BufferedBuffer> bones;

    // if the shader/mesh combo supports animations, stores offsets into the bone buffer for each object
    std::optional<BufferedBuffer> boneOffsetBuffer;

    // Expands vertices and indices so that they can contain at minimum meshIndexEnd. 
    // Works by doubling the current capacity until it fits.
        // TODO: is that really the strat?
    void ExpandVertexCapacity();

    // Expands instances so that they can contain at minimum instanceEnd. 
    // Works by doubling the current capacity until it fits.
        // TODO: is that really the strat?
    void ExpandInstanceCapacity();

    // Returns an index to the DrawCommandBuffer for the requested shader/material combo, creating the buffer if it doesn't already exist.
    CheckedUint GetCommandBuffer(const std::shared_ptr<ShaderProgram>& shader, const std::shared_ptr<Material>& material);

    friend class Mesh;
};
