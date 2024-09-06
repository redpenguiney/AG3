#include "meshpool.hpp"
#include "GL/glew.h"
#include "mesh.hpp"

#include <algorithm>
#include "../debug/assert.hpp"

Meshpool::Meshpool(const MeshVertexFormat& meshVertexFormat) :
    format(meshVertexFormat),

    vertexSize(format.GetNonInstancedVertexSize()),
    instanceSize(format.GetInstancedVertexSize()),

    vertices(GL_ARRAY_BUFFER, MESH_BUFFERING_FACTOR, 0),
    indices(GL_ELEMENT_ARRAY_BUFFER, MESH_BUFFERING_FACTOR, 0),
    instances(GL_ARRAY_BUFFER, INSTANCED_VERTEX_BUFFERING_FACTOR, 0),
    drawCommands(),
    bones(std::nullopt),
    boneOffsetBuffer(std::nullopt),

    vaoId(0),

    currentVertexCapacity(0),
    currentInstanceCapacity(0),
    //currentDrawCommandCapacity(0),

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

std::vector<Meshpool::DrawHandle> Meshpool::AddObject(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material, const std::shared_ptr<ShaderProgram>& shader, unsigned int count)
{
    // find valid slot for mesh
    unsigned int slot;
    {
        // see if this mesh is already in the pool
        if (meshUsers.contains(mesh->meshId)) {
            slot = meshUsers.at(mesh->meshId);
        }
        else {
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

            meshUsers[mesh->meshId] = slot;
            meshSlotContents[slot] = MeshSlotUsageInfo{
                .meshId = unsigned(mesh->meshId),
                .sizeClass = powerOfTwo,
                .nUsers = 0
            };
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

        DrawCommandBuffer& commandBuffer = GetCommandBuffer(shader, material);

        // find slot for draw command
        unsigned int drawCommandIndex = commandBuffer.GetNewDrawCommandSlot();

        IndirectDrawCommand command = {
            .count = static_cast<unsigned int>(mesh->indices.size()),
            .instanceCount = nInstances,
            .firstIndex = slot * (vertexSize / indexSize), /// TODO MIGHT BE WRONG 
            .baseVertex = static_cast<int>(slot),
            .baseInstance = firstInstance
        };

        commandBuffer.clientCommands[drawCommandIndex] = command;

        // fortunately, the last added one will take precedence when multiple of these update the same command
        commandBuffer.commandUpdates.emplace_back(IndirectDrawCommandUpdate{
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

            instanceSlotsToCommands[firstInstance + i] = CommandLocation{ .drawCommandIndex = drawCommandIndex};
        }
    }
    

    return ret;
}

void Meshpool::RemoveObject(const DrawHandle& handle)
{
    // something else can use this instance
    availableInstanceSlots.push_back(handle.instanceSlot);

    auto& drawBuffer = drawCommands[handle.drawBufferIndex].value();

    unsigned int originalNInstances = drawBuffer.clientCommands[instanceSlotsToCommands[handle.instanceSlot].drawCommandIndex].instanceCount;
    IndirectDrawCommand emptyCommand(0, 0, 0, 0, 0);

    // to remove the object, we need to remove its draw command, or, if it's one of multiple instances being drawn in a single command, we have to split that command up.
    if (originalNInstances == 1) {

        // mark the draw command slot as free
        drawBuffer.availableDrawCommandSlots.push_back(instanceSlotsToCommands[handle.instanceSlot].drawCommandIndex);

        drawBuffer.clientCommands[instanceSlotsToCommands[handle.instanceSlot].drawCommandIndex] = emptyCommand;
        drawBuffer.commandUpdates.emplace_back(IndirectDrawCommandUpdate{
            .updatesLeft = INSTANCED_VERTEX_BUFFERING_FACTOR,
            .command = emptyCommand,
            .commandSlot = instanceSlotsToCommands[handle.instanceSlot].drawCommandIndex
        });
        drawBuffer.drawCount--;

        if (drawBuffer.drawCount == 0) { // then we just took out the last thing being drawn in this indirect draw buffer, delete it
            drawCommands[handle.drawBufferIndex] = std::nullopt;
        }

        if (--meshSlotContents.at(handle.meshIndex).nUsers == 0) { // decrement count and if we just took out the last command using this mesh, then we should free it up
            meshSlotContents.erase(handle.meshIndex);
            availableMeshSlots[meshSlotContents.at(handle.meshIndex).sizeClass].push_back(handle.meshIndex);
            meshSlotContents.erase(meshSlotContents.at(handle.meshIndex).meshId);
        }
    }
    else {
        // then this the first instance out of multiple being removed

        unsigned int firstIndex = instanceSlotsToCommands[handle.instanceSlot].drawCommandIndex;

        IndirectDrawCommand firstHalf = drawBuffer.clientCommands[firstIndex];
        IndirectDrawCommand secondHalf = firstHalf;
        
        firstHalf.instanceCount = handle.instanceSlot - firstHalf.baseInstance;

        if (firstHalf.instanceCount + 1 < originalNInstances) {
            secondHalf.baseInstance = handle.instanceSlot + 1;
            secondHalf.instanceCount = originalNInstances - firstHalf.instanceCount - 1;
        }
        else {
            secondHalf.instanceCount = 0;
        }

        if (firstHalf.instanceCount == 0) {
            std::swap(firstHalf, secondHalf);
            if (firstHalf.instanceCount == 0) {
                firstHalf = emptyCommand;
            }
        }

            
        drawBuffer.clientCommands[firstIndex] = firstHalf;

        // fortunately, the last added one will take precedence when multiple of these update the same command
        drawBuffer.commandUpdates.emplace_back(IndirectDrawCommandUpdate{
            .updatesLeft = INSTANCED_VERTEX_BUFFERING_FACTOR,
            .command = firstHalf,
            .commandSlot = firstIndex
        });

        if (secondHalf.instanceCount != 0) {
            // find slot for draw command
            unsigned int secondIndex = drawBuffer.GetNewDrawCommandSlot();
             
            drawBuffer.clientCommands[secondIndex] = secondHalf;

            // fortunately, the last added one will take precedence when multiple of these update the same command
            drawBuffer.commandUpdates.emplace_back(IndirectDrawCommandUpdate{
                .updatesLeft = INSTANCED_VERTEX_BUFFERING_FACTOR,
                .command = secondHalf,
                .commandSlot = secondIndex
            });

            // TODO this makes me sad because O(n)
            for (unsigned int i = secondHalf.baseInstance; i < secondHalf.instanceCount; i++) {
                instanceSlotsToCommands[i].drawCommandIndex = secondIndex;
            }
        }
    }
}

void Meshpool::SetNormalMatrix(const DrawHandle& handle, const glm::mat3x3& normal)
{
    Assert(format.attributes.normalMatrix->instanced == true);
    glm::mat3x3* normalMatrixLocation = (glm::mat3x3*)(format.attributes.normalMatrix->offset + instances.Data() + (handle.instanceSlot * instanceSize));
    
    // make sure we don't segfault 
    Assert(handle.instanceSlot < currentInstanceCapacity);
    Assert((char*)normalMatrixLocation <= instances.Data() + (instanceSize * currentInstanceCapacity));
    Assert((char*)normalMatrixLocation >= instances.Data());
    *normalMatrixLocation = normal;
}

void Meshpool::SetModelMatrix(const DrawHandle& handle, const glm::mat4x4& model)
{
    Assert(format.attributes.modelMatrix->instanced == true);
    glm::mat4x4* modelMatrixLocation = (glm::mat4x4*)(format.attributes.modelMatrix->offset + instances.Data() + (handle.instanceSlot * instanceSize));

    // make sure we don't segfault 
    Assert(handle.instanceSlot < currentInstanceCapacity);
    Assert((char*)modelMatrixLocation <= instances.Data() + (instanceSize * currentInstanceCapacity));
    Assert((char*)modelMatrixLocation >= instances.Data());
    *modelMatrixLocation = model;
}

void Meshpool::SetBoneState(const DrawHandle& handle, unsigned int nBones, glm::mat4x4* offsets)
{
    Assert(format.supportsAnimation);
        
    Assert(format.maxBones <= nBones);
    glm::mat4x4* bonesLocation = (glm::mat4x4*)(bones->Data() + handle.instanceSlot * sizeof(glm::mat4x4));
    
    // make sure we don't segfault 
    Assert(instanceId < currentInstanceCapacity);
    Assert(slot < meshCapacity);
    Assert((char*)bonesLocation <= bones->Data() + (sizeof(glm::mat4x4) * currentInstanceCapacity));
    Assert((char*)bonesLocation >= bones->Data());
    
    // copy in bone transforms
    memcpy(bonesLocation, offsets, nBones * sizeof(glm::mat4x4));
}

void Meshpool::Draw() {
    glBindVertexArray(vaoId);
    indices.Bind();
    if (bones.has_value()) {
        bones->BindBase(BONE_BUFFER_BINDING);
        boneOffsetBuffer->BindBase(BONE_OFFSET_BUFFER_BINDING);
    }
    
    //double start1 = Time();
    //glPointSize(4.0);
    for (auto& command : drawCommands) {
        if (!command.has_value()) { continue; }
        command->buffer.Bind();
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)command->buffer.GetOffset(), command->drawCount, 0);
    }
     
    
}

void Meshpool::Commit() {
    // write indirect draw commands to buffer
    for (auto& drawBuffer : drawCommands) {
        if (!drawBuffer.has_value()) { continue; }
        for (unsigned int i = 0; i < drawBuffer->commandUpdates.size(); i++) {
            auto& update = drawBuffer->commandUpdates[i];
            Assert(update.updatesLeft != 0);
            update.updatesLeft--;

            IndirectDrawCommand command = update.command; // deliberate copy

            command.baseInstance += instances.GetOffset() / instanceSize;
            command.baseVertex += vertices.GetOffset() / vertexSize;

            memcpy(drawBuffer->buffer.Data() + (update.commandSlot * sizeof(IndirectDrawCommand)), &command, sizeof(IndirectDrawCommand));

            if (update.updatesLeft == 0) {
                drawBuffer->commandUpdates[i] = drawBuffer->commandUpdates.back();
                drawBuffer->commandUpdates.pop_back();
                i--;
                //DebugLogInfo("bye bye")
            }
        }

        drawBuffer->buffer.Commit();
    }
    
    vertices.Commit();
    instances.Commit();
    indices.Commit();
    
    if (bones) {
        bones->Commit();
        boneOffsetBuffer->Commit();
    }
}

void Meshpool::FlipBuffers()
{
    vertices.Flip();
    instances.Flip();
    indices.Flip();
    for (auto& c : drawCommands) {
        if (!c.has_value()) { continue; }
        c->buffer.Flip();
    }
    if (bones) {
        bones->Flip();
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

void Meshpool::DrawCommandBuffer::ExpandDrawCountCapacity()
{
    // update capacity
    unsigned int oldCapacity = currentDrawCommandCapacity;
    if (currentDrawCommandCapacity == 0) {
        currentDrawCommandCapacity = 16;
    }
    else {
        currentDrawCommandCapacity *= 2;
    }

    clientCommands.resize(currentDrawCommandCapacity, IndirectDrawCommand(0, 0, 0, 0, 0));
    

    // expand draw command buffer
    buffer.Reallocate(currentDrawCommandCapacity * sizeof(IndirectDrawCommand));

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

    instanceSlotsToCommands.resize(currentInstanceCapacity, Meshpool::CommandLocation(0));

    // Update buffers.
    instances.Reallocate(currentInstanceCapacity * instanceSize);
    if (format.supportsAnimation) {
        bones->Reallocate(currentInstanceCapacity * sizeof(glm::mat4x4) * format.maxBones);
        boneOffsetBuffer->Reallocate(currentInstanceCapacity * sizeof(GLuint));
    }

    // Associate data with the vao and describe format of instanced data
    Assert(vaoId != 0);
    instances.Bind();
    format.SetInstancedVaoVertexAttributes(vaoId, instanceSize, vertexSize);
}

Meshpool::DrawCommandBuffer& Meshpool::GetCommandBuffer(const std::shared_ptr<ShaderProgram>& shader, const std::shared_ptr<Material>& material)
{
    for (auto& buffer : drawCommands) {
        if (buffer.has_value() && buffer->shader == shader && buffer->material == material) {
            return *buffer;
        }
    }

    // if none was found, make a buffer for this material/shader combo
    DrawCommandBuffer b{
        .shader = shader,
        .material = material,
        .buffer = std::move(BufferedBuffer(GL_DRAW_INDIRECT_BUFFER, INSTANCED_VERTEX_BUFFERING_FACTOR, 0)),
    };

    if (availableDrawCommandBufferIndices.size()) {
        drawCommands.emplace(drawCommands.begin() + availableDrawCommandBufferIndices.back(), std::move(b));
    }
    else {
        drawCommands.emplace_back(std::move(b));
    }
}

unsigned int Meshpool::DrawCommandBuffer::GetNewDrawCommandSlot()
{
    if (availableDrawCommandSlots.size() == 0) {
        ExpandDrawCountCapacity();
    }
    unsigned drawCommandIndex = availableDrawCommandSlots.back();
    availableDrawCommandSlots.pop_back();
    return drawCommandIndex;
}
