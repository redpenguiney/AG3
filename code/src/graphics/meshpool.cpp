#include "meshpool.hpp"
#include "GL/glew.h"
#include "mesh.hpp"

#include <algorithm>

Meshpool::Meshpool(const MeshVertexFormat& meshVertexFormat) :
    format(meshVertexFormat),

    vertexSize(format.GetNonInstancedVertexSize()),
    instanceSize(format.GetInstancedVertexSize()),

    vertices(GL_ARRAY_BUFFER, MESH_BUFFERING_FACTOR, 0),
    indices(GL_ELEMENT_ARRAY_BUFFER, MESH_BUFFERING_FACTOR, 0),
    instances(GL_ARRAY_BUFFER, INSTANCED_VERTEX_BUFFERING_FACTOR, 0),
    drawCommands(GL_DRAW_INDIRECT_BUFFER, INSTANCED_VERTEX_BUFFERING_FACTOR, 0),
    boneBuffer(std::nullopt),
    boneOffsetBuffer(std::nullopt),

    vaoId(0),

    drawCount(0),
    currentVertexCapacity(0),
    currentInstanceCapacity(0),
    currentDrawCommandCapacity(0),

    meshIndexEnd(0),
    instanceEnd(0)
{

}

Meshpool::~Meshpool()
{
	if (vaoId != 0) {
		glDeleteVertexArrays(1, &vaoId);
	}
}

std::vector<Meshpool::DrawHandle> Meshpool::AddObject(const std::shared_ptr<Mesh>& mesh, unsigned int count)
{
    // find valid slot for mesh
    unsigned int slot;
    {
        // for simplicity we like to assume indices and vertices are the same size in bytes and pad up the difference
        unsigned int meshNBytes = std::max(mesh->vertices.size() * vertexSize, mesh->indices.size() * indexSize);

        unsigned int powerOfTwo = vertexSize;
        while (powerOfTwo < meshNBytes) {
            meshNBytes *= 2;
        }

        // first, check availableMeshSlots
        if (availableMeshSlots.at(powerOfTwo).size() > 0) {
            slot = availableMeshSlots[powerOfTwo].back();
            availableMeshSlots[powerOfTwo].pop_back();
        }
        // if they've got nothing, take a slot from the end of the vertices buffer
        else {
            slot = meshIndexEnd;
            meshIndexEnd += powerOfTwo / vertexSize;

            // make sure vertices has room. (vaoId will be 0 if this is the first object being put in the pool) 
            if (meshIndexEnd >= currentVertexCapacity || vaoId == 0) {
                ExpandVertexCapacity();
            }
        }
    }
    
    
    meshUpdates.emplace_back(MeshUpdate { MESH_BUFFERING_FACTOR, mesh, slot });

    std::vector<DrawHandle> ret;
    unsigned int nCreated = 0;
    while (nCreated < count) {
        unsigned int nInstances, firstInstance;
        if (availableInstanceSlots.size() != 0) {
            nInstances = 1;
            firstInstance = availableInstanceSlots.back();
            availableInstanceSlots.pop_back();

            
        }
        else {
            firstInstance = instanceEnd;
            nInstances = count - nCreated;

            instanceEnd += nInstances;
            ExpandInstanceCapacity();
        }

        // find slot for draw command
        if (availableDrawCommandSlots.size() == 0) {
            ExpandDrawCountCapacity();
        }
        unsigned drawCommandIndex = availableDrawCommandSlots.back();
        availableDrawCommandSlots.pop_back();

        IndirectDrawCommand command = {
            .count = static_cast<unsigned int>(mesh->indices.size()),
            .instanceCount = nInstances,
            .firstIndex = slot, /// TODO MIGHT BE WRONG 
            .baseVertex = static_cast<int>(slot),
            .baseInstance = firstInstance
        };

        commandUpdates.emplace_back(IndirectDrawCommandUpdate{
            .updatesLeft = INSTANCED_VERTEX_BUFFERING_FACTOR,
            .command = command,
            .commandSlot = drawCommandIndex
        });

        nCreated += nInstances;

        for (unsigned int i = 0; i < nInstances; i++) {
            ret.emplace_back(DrawHandle{
                .meshIndex = slot,
                .instanceSlot = firstInstance + i,  
            });

            instanceSlotsToCommands[firstInstance + i] = CommandLocation{ .drawCommandIndex = drawCommandIndex, .command = command };
        }
    }
    

    return ret;
}

void Meshpool::RemoveObject(const DrawHandle& handle)
{
    // something else can use this instance
    availableInstanceSlots.push_back(handle.instanceSlot);

    // to remove the object, we need to remove its draw command, or, if it's one of multiple instances being drawn in a single command, we have to split that command up.
    if (instanceSlotsToCommands[handle.instanceSlot].command.instanceCount == 1) {

        // mark the draw command slot as free
        availableDrawCommandSlots.push_back(instanceSlotsToCommands[handle.instanceSlot].drawCommandIndex);
        
        instanceSlotsToCommands

        commandUpdates.emplace_back(IndirectDrawCommandUpdate{
            .updatesLeft = INSTANCED_VERTEX_BUFFERING_FACTOR,
            .commandSlot = instanceSlotsToCommands[handle.instanceSlot].drawCommandIndex
        });
    } 
    else if (instanceSlotsToCommands[handle.instanceSlot].instanceId == handle.instanceSlot) {
        // then this the first instance out of multiple being removed
    }
}

void Meshpool::Draw() {
    glBindVertexArray(vaoId);
    indices.Bind();
    drawCommands.Bind();
    if (boneBuffer.has_value()) {
        boneBuffer->BindBase(BONE_BUFFER_BINDING);
        boneOffsetBuffer->BindBase(BONE_OFFSET_BUFFER_BINDING);
    }
    
    //double start1 = Time();
    //glPointSize(4.0);
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)drawCommands.GetOffset(), drawCount, 0); // TODO: GET INDIRECT DRAWING TO WORK    
    
}

void Meshpool::Commit() {
    // write indirect draw commands to buffer
    for (unsigned int i = 0; i < commandUpdates.size(); i++) {
        auto& update = commandUpdates[i];
        Assert(update.updatesLeft != 0);
        update.updatesLeft--;

        IndirectDrawCommand command = update.command; // deliberate copy
        
        command.baseInstance += instances.GetOffset() / instanceSize;
        command.baseVertex += vertices.GetOffset() / vertexSize;

        memcpy(drawCommands.Data() + (update.commandSlot * sizeof(IndirectDrawCommand)), &command, sizeof(IndirectDrawCommand));

        if (update.updatesLeft == 0) {
            commandUpdates[i] = commandUpdates.back();
            commandUpdates.pop_back();
            i--;
            //DebugLogInfo("bye bye")
        }
    }

    vertices.Commit();
    instances.Commit();
    indices.Commit();
    drawCommands.Commit();
    if (boneBuffer) {
        boneBuffer->Commit();
        boneOffsetBuffer->Commit();
    }
}

void Meshpool::FlipBuffers()
{
    vertices.Flip();
    instances.Flip();
    indices.Flip();
    drawCommands.Flip();
    if (boneBuffer) {
        boneBuffer->Flip();
        boneOffsetBuffer->Flip();
    }
}

void Meshpool::ExpandVertexCapacity()
{
    // determine new vertex capacity
    while (currentVertexCapacity <= meshIndexEnd) {
        currentVertexCapacity *= 2;
    }

    // resize buffers
    vertices.Reallocate(currentVertexCapacity * vertexSize);
    indices.Reallocate(currentVertexCapacity * indexSize);

    // delete old vao
    if (vaoId != 0) {
        glDeleteVertexArrays(1, &vaoId);
    }

    // make new vao
    glGenVertexArrays(1, &vaoId);
    glBindVertexArray(vaoId);
    vertices.Bind();
    format.SetNonInstancedVaoVertexAttributes(vaoId, instanceSize, vertexSize);
    
    // because we just recreated the vao, we have to rebind the instanced attributes too 
    // but when we're initializing, we don't want to do this because calling ExpandInstanced() in initializiation will and we don't have an instanced vertex buffer yet
    if (instances.bufferId != 0) {
        instances.Bind();
        format.SetInstancedVaoVertexAttributes(vaoId, instanceSize, vertexSize);
    }
}

void Meshpool::ExpandDrawCountCapacity()
{
    // update capacity
    unsigned int oldCapacity = currentDrawCommandCapacity;
    if (currentDrawCommandCapacity == 0) {
        currentDrawCommandCapacity = 16;
    }
    else {
        currentDrawCommandCapacity *= 2;
    }

    // expand draw command buffer
    drawCommands.Reallocate(currentDrawCommandCapacity * sizeof(IndirectDrawCommand));

    // add new instance slots
    for (unsigned int i = currentDrawCommandCapacity; i --> oldCapacity;) {
        availableDrawCommandSlots.push_back(i);
    }
    std::sort(availableDrawCommandSlots.begin(), availableDrawCommandSlots.end(), std::greater<unsigned int>());
}

void Meshpool::ExpandInstanceCapacity()
{
    // determine new instance capacity
    while (currentInstanceCapacity <= instanceEnd) {
        currentInstanceCapacity *= 2;
    }

    // Update buffers.
    instances.Reallocate(currentInstanceCapacity * instanceSize);
    if (format.supportsAnimation) {
        boneBuffer->Reallocate(currentInstanceCapacity * sizeof(glm::mat4x4) * format.maxBones);
        boneOffsetBuffer->Reallocate(currentInstanceCapacity * sizeof(GLuint));
    }

    // Associate data with the vao and describe format of instanced data
    Assert(vaoId != 0);
    instances.Bind();
    format.SetInstancedVaoVertexAttributes(vaoId, instanceSize, vertexSize);
}
