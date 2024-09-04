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

const unsigned int INSTANCED_VERTEX_BUFFERING_FACTOR = 3;
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
        const unsigned int meshIndex;

        // An index (in terms of instanceSize) to where the object's instance data is stored in the instances buffer.
        const unsigned int instanceSlot;

        // Index into drawCommands, which INDBO the object's draw command is stored in.
        const unsigned int drawBufferIndex;
    };

    // Only meshes with this format can be stored in this meshpool.
    const MeshVertexFormat format;

    Meshpool(const MeshVertexFormat& meshVertexFormat);

    Meshpool(const Meshpool&) = delete; // try to copy construct and i will end you

    ~Meshpool();

    // Adds count identical objects to the pool with the given mesh, and returns a DrawHandle for each object.
    std::vector<DrawHandle> AddObject(const std::shared_ptr<Mesh>&, const std::shared_ptr<Material>&, const std::shared_ptr<ShaderProgram>&, unsigned int count);

    // Frees the given object from the meshpool, so something else can use that space.
    void RemoveObject(const DrawHandle& handle);

    // Makes the given instance use the given normal matrix.
    // Will abort if mesh uses per-vertex normal matrix instead of per-instance normal matrix. (though who would do that???)
    void SetNormalMatrix(const DrawHandle& handle, const glm::mat3x3& normal);

    // Makes the given instance use the given model matrix.
    // Will abort if mesh uses per-vertex model matrix instead of per-instance model matrix. (though who would do that???)
    void SetModelMatrix(const DrawHandle& handle, const glm::mat4x4& model);

    // Sets the bone transforms. Do not call if the meshpool vertex format does not support animation.
    void SetBoneState(const DrawHandle& handle, unsigned int nBones, glm::mat4x4* offsets);

    // Set the given instanced vertex attribute of the given instance to the given value.
    // Will abort if nFloats does not match the mesh's vertex format or if the vertex attribute is not instanced.
    template<unsigned int nFloats>
    void SetInstancedVertexAttribute(const DrawHandle& handle, const unsigned int attributeName, const glm::vec<nFloats, float>& value);

    // idk what to put here, you probably know what this does
    void Draw();

    // needed for BufferedBuffer's double/triple buffering, call every frame AFTER writing vertex/instance data and BEFORE calling Draw().
    void Commit();

    // needed for BufferedBuffer's double/triple buffering, call every frame BEFORE writing vertex/instance data and AFTER calling Draw(). 
    // Might yield if GPU isn't ready for us to write the data, so call at the last possible second.
    void FlipBuffers();


private:
    inline static const unsigned int BONE_BUFFER_BINDING = 2;
    inline static const unsigned int BONE_OFFSET_BUFFER_BINDING = 3;

    struct MeshUpdate {
        unsigned int updatesLeft;
        std::shared_ptr<Mesh> mesh;
        unsigned int meshIndex;
    };

    struct CommandLocation {
        unsigned int drawCommandIndex;

        // Here, command.baseInstance and command.baseVertex presume no multiple buffering. Meshpool::Commit() corrects that.
        //IndirectDrawCommand command;
    };

    struct SlotUsageInfo {
        // id of the mesh inside this slot
        unsigned int meshId;
    };

    class DrawCommandBuffer {
    public:
        const std::shared_ptr<ShaderProgram> shader;
        const std::shared_ptr<Material> material;
        BufferedBuffer buffer;

        // number of commands in buffer
        unsigned int drawCount = 0;

        unsigned int currentDrawCommandCapacity = 0;

        // unlike the other two available<x>slots, this one does hold every slot that isn't taken and if this is empty, then you have to expand.
    // Sorted from greatest to least.
        std::vector<unsigned int> availableDrawCommandSlots = {};

        std::vector<IndirectDrawCommandUpdate> commandUpdates = {};

        // all 0s for empty/available command slots
        std::vector<IndirectDrawCommand> clientCommands = {}; // equivelent contents to drawCommands, but we can't read from that because its a GPU buffer
    
        unsigned int GetNewDrawCommandSlot();

        // Doubles currentDrawCommandCapacity.
        void ExpandDrawCountCapacity();
    };

    // the size of a single instance for a single object in bytes. Equal to the InstancedSize() of the vertex format.
    const unsigned int instanceSize; 

    // the size of a single vertex for a single mesh in bytes. Equal to the NonInstancedSize() of the vertex format.
    const unsigned int vertexSize;

    constexpr static unsigned int indexSize = sizeof(GLuint);

    unsigned int currentVertexCapacity;
    unsigned int currentInstanceCapacity;
 

    

    // if you want to allocate memory for a mesh more than vertexSize * 2^(n-1) bytes but less than vertexSize * 2^n bytes, the nth vector in this array has room for that.
    // When a mesh is removed from the pool, an index to its memory goes here.
    // The unsigned ints in each vector are vertex indices (same unit as DrawHandle.meshIndex)
    std::array<std::vector<unsigned int>, 32> availableMeshSlots;

    // if availableMeshSlots is empty, this is the mesh index of the first available part of the vertices buffer
    unsigned int meshIndexEnd;
    
    // For each mesh slot, describes meshId.
    std::vector<SlotUsageInfo> meshSlotContents;

    std::vector<unsigned int> availableInstanceSlots;
    unsigned int instanceEnd;

    
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
    std::vector<DrawCommandBuffer> drawCommands;
    std::vector<unsigned int> availableDrawCommandBufferIndices;

	// if the shader/mesh combo supports animations, stores the bone transform matrices (and the number of them)
	std::optional<BufferedBuffer> boneBuffer; 

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

    // Returns a reference to the DrawCommandBuffer for the requested shader/material combo, creating the buffer if it doesn't already exist.
    DrawCommandBuffer& GetCommandBuffer(const std::shared_ptr<ShaderProgram>& shader, const std::shared_ptr<Material>& material);
};