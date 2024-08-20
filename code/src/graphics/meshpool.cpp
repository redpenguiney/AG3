#include "GL/glew.h"
#include <glm/vec3.hpp>
#include "GLM/gtx/string_cast.hpp"
#include "debug/debug.hpp"
#include "debug/assert.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "debug/log.hpp"
#include "meshpool.hpp"

// TODO: use GL_SHORT for indices in meshpools where there are fewer than 65536 indices (wait is that even possible)
// TODO: we assume indices size will never exceed vertices size which is very dangerous

Meshpool::Meshpool(const MeshVertexFormat& meshVertexFormat, unsigned int numVertices): 
    instancedVertexSize((Assert(meshVertexFormat.GetInstancedVertexSize() > 0), meshVertexFormat.GetInstancedVertexSize())), // this is use of comma operator to do sanity check because we had bad values in ehre somehow
    nonInstancedVertexSize(meshVertexFormat.GetNonInstancedVertexSize()), 
    meshVerticesSize(((((int)std::pow(2, 1 + (int)std::log2(numVertices * nonInstancedVertexSize))) + nonInstancedVertexSize - 1)/nonInstancedVertexSize) * nonInstancedVertexSize), // makes meshVerticesSize a power of two rounded to the nearest multiple of vertexSize (must be multiple of vertexSize for OpenGL base vertex argument to work)
    meshIndicesSize(meshVerticesSize),
    vertexFormat(meshVertexFormat),
    baseMeshCapacity((Assert(meshVerticesSize != 0), (TARGET_VBO_SIZE / meshVerticesSize) + 1)), // +1 just in case the base capacity was somehow 0
    baseInstanceCapacity((TARGET_VBO_SIZE/instancedVertexSize) + 1),

    // TOM REMEMEMBER: IF YOU INCREASE BUFFER COUNT FOR VERTEX BUFFER, YOU NEED TO FILL ALL OF THEM IN FILLSLOT. (TODO that might be needed for stuff like minecraft chunks?)
    vertexBuffer(GL_ARRAY_BUFFER, 1, 0), 
    instancedVertexBuffer(GL_ARRAY_BUFFER, INSTANCED_VERTEX_BUFFERING_FACTOR, 0),
    indexBuffer(GL_ELEMENT_ARRAY_BUFFER, 1, 0),
    indirectDrawBuffer(GL_DRAW_INDIRECT_BUFFER, INSTANCED_VERTEX_BUFFERING_FACTOR, 0)
    
{ 
    // DebugLogInfo("SIZE is ", meshVerticesSize, " numVs was ", numVertices);

    if (meshVertexFormat.supportsAnimation) {
        Assert(meshVertexFormat.maxBones > 0);
        boneBuffer.emplace(GL_SHADER_STORAGE_BUFFER, INSTANCED_VERTEX_BUFFERING_FACTOR, 0);
        boneOffsetBuffer.emplace(GL_SHADER_STORAGE_BUFFER, 1, 0);
    }
    vao = 0;

    meshCapacity = 0;
    instanceCapacity = 0;
    drawCount = 0;

    ExpandNonInstanced();
    ExpandInstanced(1);
}

std::vector<std::pair<unsigned int, unsigned int>> Meshpool::AddObject(const unsigned int meshId, unsigned int count) {
    // std::printf("\tBe advised: adding %u of %u\n", count, meshId);
    std::vector<std::pair<unsigned int, unsigned int>> objLocations;
    unsigned int meshInstanceCapacity = Mesh::Get(meshId)->instanceCount;

    

    // if no value for this key, will be automatically created as an empty vector
    auto& contents = slotContents[meshId];
    
    // if any slots for this mesh have room for more instances, try to fill them up first
    for (unsigned int slot : contents) {
        // std::cout << "\tChecking slot " << slot << ".\n";
        // Check if RemoveObject() created any spaces to put new instances in
        if (slotInstanceSpaces.count(slot)) {
            // std::cout << "\tSlot instance spaces has room.\n";
            unsigned int len = slotInstanceSpaces[slot].size();
            for (unsigned int i = 0; i < std::min(len, count); i++) {
                auto instance = *(slotInstanceSpaces[slot].begin());
                objLocations.push_back(std::make_pair(slot, instance));

                slotInstanceSpaces[slot].erase(instance);

                if (vertexFormat.supportsAnimation) { // tell shader where its bones are stored
                    *(boneOffsetBuffer->Data() + sizeof(GLuint) * (slotToInstanceLocations[slot] + instance)) = (slotToInstanceLocations[slot] + instance);
                }        
            }

            // totally remove the map entry if we used all the spaces
            if (len == count) {
                slotInstanceSpaces.erase(slot);
            }

            count -= std::min(len, count);

            // if we added all of the requested objects then we're done here
            if (count == 0) {
                return objLocations;
            }
        }

        // Otherwise append instances to the end of the slot
        unsigned int storedCount = drawCommands[slot].instanceCount;
        if (storedCount < meshInstanceCapacity) { // if this slot has room
            // std::cout << "\tSlot has room at end.\n";
            const unsigned int space = meshInstanceCapacity - storedCount;

            // add positions to objLocations
            for (unsigned int instanceId = drawCommands[slot].instanceCount; instanceId < drawCommands[slot].instanceCount + std::min(count, space); instanceId++) {
                objLocations.push_back(std::make_pair(slot, instanceId));

                if (vertexFormat.supportsAnimation) { // tell shader where its bones are stored
                    *(boneOffsetBuffer->Data() + sizeof(GLuint) * (slotToInstanceLocations[slot] + instanceId)) = (slotToInstanceLocations[slot] + instanceId);
                } 
            }

            drawCommands[slot].instanceCount += std::min(count, space);

            count -= std::min(count, space);

            // if we added all of the requested objects then we're done here
            if (count == 0) {
                return objLocations;
            }
        }
    }

    // failing that, create slots and fill them until count is 0
    while (count > 0) {
        // std::cout << "\tInsufficient room after all that, creating slot.\n";
        if (availableMeshSlots.size() == 0) { // expand the meshpool if there isn't room for the new mesh
            // std::cout << "\tExpanding...\n";
            ExpandNonInstanced();
            // std::cout << "\tExpanded.\n";
        }
        // std::cout << "\tChecking for slots with matching instance capacity.\n";
        // first try to get slot with matching instance capacity, failing that do a whole new slot
        unsigned int slot;
        if (availableMeshSlots.count(meshInstanceCapacity)) {
            // std::cout << "\tFound one.\n";
            slot = availableMeshSlots[meshInstanceCapacity].front();
            availableMeshSlots[meshInstanceCapacity].pop_front();
        }
        else {
            // std::cout << "\tDid not find one, reserving slot for this mesh.\n";
            slot = availableMeshSlots[0].front();
            availableMeshSlots[0].pop_front();
        }

        // std::cout << "\tSlot obtained.\n";
        unsigned int start = drawCommands[slot].instanceCount;
        FillSlot(meshId, slot, std::min(count, meshInstanceCapacity), false);
        // std::cout << "\tFilled slot.\n";
        drawCount += 1;

        // add positions to objLocations
        for (unsigned int instanceId = start; instanceId < start + std::min(count, meshInstanceCapacity); instanceId++) {
            objLocations.push_back(std::make_pair(slot, instanceId));

            if (vertexFormat.supportsAnimation) { // tell shader where its bones are stored
                    *(boneOffsetBuffer->Data() + sizeof(GLuint) * (slotToInstanceLocations[slot] + instanceId)) = (slotToInstanceLocations[slot] + instanceId);
                } 
        }
        // std::cout << "\tSet objLocations.\n";

        //drawCommands[slot].instanceCount = std::min(count, instanceCapacity);
        count -= std::min(count, meshInstanceCapacity);
    }
    
    return objLocations;
}

void Meshpool::RemoveObject(const unsigned int slot, const unsigned int instanceId) {    
    // don't remove all these debug print statements, i have a feeling we'll need them again soon

    // make sure instanceId is valid
    // TODO: check slot is valid
    //auto t = Time();
    //std::printf("\nMy guy %u %u %u", drawCommands[slot].instanceCount, instanceId, slot);
    Assert(drawCommands[slot].instanceCount > instanceId);
   
   // if the instance is at the end of that slot we can just do this the easy way
    if (drawCommands[slot].instanceCount == instanceId + 1) {
        drawCommands.at(slot).instanceCount -= 1;
        //std::printf("\n\tRemoved %u because it was on the end, count now %u", instanceId, drawCommands[slot].instanceCount);
        //std::printf("\n\tinstanceSpaces[slot %u] contains %u", slot, slotInstanceSpaces[slot].size());

        // if the slot right before this one (and ones before it, given we don't remove ALL instances) are marked as empty, we should free them from being pointlessly drawn
        for (unsigned int instanceToRemove = instanceId - 1; (instanceToRemove > 0) && slotInstanceSpaces[slot].count(instanceToRemove); instanceToRemove--) {
            
            slotInstanceSpaces[slot].erase(instanceToRemove);
            drawCommands[slot].instanceCount -= 1;
            //std::printf("\n\tAlso able to remove %u, count now %u", instanceToRemove, drawCommands[slot].instanceCount);
        }
    }
    else {
        // otherwise we have to just mark the instance as empty
        //std::printf("\n\tPushing %u to slotInstanceSpaces[slot %u]", instanceId, slot);
        SetModelMatrix(slot, instanceId, glm::mat4x4(0)); // make it so it can't be drawn
        slotInstanceSpaces[slot].insert(instanceId); // tell AddObject that this instance can be overwritten
    }
    
   
    // if we removed all instances from the slot, mark the slot as empty and available for another mesh to use
    const unsigned int actualInstancesInSlot = drawCommands[slot].instanceCount - ((slotInstanceSpaces.count(slot)) ? slotInstanceSpaces[slot].size() : 0);
    if (actualInstancesInSlot == 0) {
        availableMeshSlots[slotInstanceReservedCounts[slot]].push_back(slot);
        drawCommands.at(slot).instanceCount = 0;
        drawCommands.at(slot).count = 0;
        //std::printf("\nWE DONE WITH SLOT %u", slot);
    }

    //LogElapsed(t);
}

// void Meshpool::SetColor(const unsigned int slot, const unsigned int instanceId, const glm::vec4& rgba) {
//     // make sure this instance slot hasn't been deleted
//     if (slotInstanceSpaces.count(slot)) {
//         Assert(!slotInstanceSpaces[slot].count(instanceId));
//     }

//     Assert(vertexFormat.attributes.color->instanced == true);
//     glm::vec4* colorLocation = (glm::vec4*)(vertexFormat.attributes.color->offset + instancedVertexBuffer.Data() + ((slotToInstanceLocations[slot] + instanceId) * instancedVertexSize));

//     // make sure we don't segfault 
//     Assert((char*)colorLocation + sizeof(glm::vec4) <= instancedVertexBuffer.Data() + (instancedVertexSize * instanceCapacity)); 
//     Assert((char*)colorLocation >= instancedVertexBuffer.Data());
    
//     memcpy(colorLocation, &rgba, vertexFormat.attributes.color->nFloats * sizeof(GLfloat));
//     // *colorLocation = rgba;
// }

// void Meshpool::SetTextureZ(const unsigned int slot, const unsigned int instanceId, const float textureZ) {
//     // make sure this instance slot hasn't been deleted
//     if (slotInstanceSpaces.count(slot)) {
//         Assert(!slotInstanceSpaces[slot].count(instanceId));
//     }
    
//     Assert(vertexFormat.attributes.textureZ->instanced == true);
//     float* textureZLocation = (float*)(vertexFormat.attributes.textureZ->offset + instancedVertexBuffer.Data() + ((slotToInstanceLocations[slot] + instanceId) * instancedVertexSize));

//     // make sure we don't segfault 
//     Assert((char*)textureZLocation + sizeof(float) <= instancedVertexBuffer.Data() + (instancedVertexSize * instanceCapacity)); 
//     Assert((char*)textureZLocation >= instancedVertexBuffer.Data());
    
//     *textureZLocation = textureZ;
// }

template <unsigned int nFloats>
void Meshpool::SetInstancedVertexAttribute(const unsigned int slot, const unsigned int instanceId, const unsigned int attributeName, const glm::vec<nFloats, float>& value) {
    // make sure this instance slot hasn't been deleted
    if (slotInstanceSpaces.count(slot)) {
        Assert(!slotInstanceSpaces[slot].count(instanceId));
    }
    
    if (!vertexFormat.vertexAttributes[MeshVertexFormat::AttributeIndexFromAttributeName(attributeName)].has_value()) {
        DebugLogError("The current meshpool's meshvertexformat has no such attribute ", attributeName, ", and thus the value at slot ", slot, " instanceId ", instanceId, " could not be set.");
        abort();
    }
    Assert(vertexFormat.vertexAttributes[MeshVertexFormat::AttributeIndexFromAttributeName(attributeName)]->instanced == true);
    Assert(vertexFormat.vertexAttributes[MeshVertexFormat::AttributeIndexFromAttributeName(attributeName)]->nFloats == nFloats);
    float* attributeLocation = (float*)(vertexFormat.vertexAttributes[MeshVertexFormat::AttributeIndexFromAttributeName(attributeName)]->offset + instancedVertexBuffer.Data() + ((slotToInstanceLocations[slot] + instanceId) * instancedVertexSize));

    // make sure we don't segfault 
    Assert((char*)attributeLocation + vertexFormat.vertexAttributes[MeshVertexFormat::AttributeIndexFromAttributeName(attributeName)]->nFloats * sizeof(GLfloat) <= instancedVertexBuffer.Data() + (instancedVertexSize * instanceCapacity)); 
    Assert((char*)attributeLocation >= instancedVertexBuffer.Data());
    
    memcpy(attributeLocation, &value, sizeof(value));
}

// explicit template instantiations
template void Meshpool::SetInstancedVertexAttribute<4>(const unsigned int slot, const unsigned int instanceId, const unsigned int attributeName, const glm::vec4&);
template void Meshpool::SetInstancedVertexAttribute<3>(const unsigned int slot, const unsigned int instanceId, const unsigned int attributeName, const glm::vec3&);
template void Meshpool::SetInstancedVertexAttribute<2>(const unsigned int slot, const unsigned int instanceId, const unsigned int attributeName, const glm::vec2&);
template void Meshpool::SetInstancedVertexAttribute<1>(const unsigned int slot, const unsigned int instanceId, const unsigned int attributeName, const glm::vec1&);

void Meshpool::SetModelMatrix(const unsigned int slot, const unsigned int instanceId, const glm::mat4x4& matrix) {
    // make sure this instance slot hasn't been deleted
    if (slotInstanceSpaces.count(slot)) {
        Assert(!slotInstanceSpaces[slot].count(instanceId));
    }
    
    Assert(vertexFormat.attributes.modelMatrix->instanced == true);
    glm::mat4x4* modelMatrixLocation = (glm::mat4x4*)(vertexFormat.attributes.modelMatrix->offset + instancedVertexBuffer.Data() + ((slotToInstanceLocations[slot] + instanceId) * instancedVertexSize));

    // make sure we don't segfault 
    Assert((char*)modelMatrixLocation <= instancedVertexBuffer.Data() + (instancedVertexSize * instanceCapacity)); 
    Assert((char*)modelMatrixLocation >= instancedVertexBuffer.Data());
    *modelMatrixLocation = matrix;
}

void Meshpool::SetNormalMatrix(const unsigned int slot, const unsigned int instanceId, const glm::mat3x3& normal) {
    // make sure this instance slot hasn't been deleted
    if (slotInstanceSpaces.count(slot)) {
        Assert(!slotInstanceSpaces[slot].count(instanceId));
    }
    
    Assert(vertexFormat.attributes.normalMatrix->instanced == true);
    glm::mat3x3* normalMatrixLocation = (glm::mat3x3*)(vertexFormat.attributes.normalMatrix->offset + instancedVertexBuffer.Data() + ((slotToInstanceLocations[slot] + instanceId) * instancedVertexSize));

    // make sure we don't segfault 
    Assert(instanceId < instanceCapacity);
    Assert(slot < meshCapacity);
    Assert((char*)normalMatrixLocation <= instancedVertexBuffer.Data() + (instancedVertexSize * instanceCapacity)); 
    Assert((char*)normalMatrixLocation >= instancedVertexBuffer.Data());
    *normalMatrixLocation = normal;
}

void Meshpool::SetBoneState(const unsigned int slot, const unsigned int instanceId, unsigned int nBones, glm::mat4x4* offsets) {
    Assert(vertexFormat.supportsAnimation);

    // make sure this instance slot hasn't been deleted
    if (slotInstanceSpaces.count(slot)) {
        Assert(!slotInstanceSpaces[slot].count(instanceId));
    }
    
    Assert(vertexFormat.attributes.modelMatrix->instanced == true);
     //DebugLogInfo("writing to offset ", ((slotToInstanceLocations[slot] + instanceId) * sizeof(glm::mat4x4)));
    glm::mat4x4* bonesLocation = (glm::mat4x4*)(boneBuffer->Data() + ((slotToInstanceLocations[slot] + instanceId) * sizeof(glm::mat4x4)));

    // make sure we don't segfault 
    Assert(instanceId < instanceCapacity);
    Assert(slot < meshCapacity);
    Assert((char*)bonesLocation <= boneBuffer->Data() + (sizeof(glm::mat4x4) * instanceCapacity)); 
    Assert((char*)bonesLocation >= boneBuffer->Data());

    // copy in bone transforms
    memcpy(bonesLocation, offsets, nBones * sizeof(glm::mat4x4));
}

void Meshpool::Draw() {
    glBindVertexArray(vao);
    indexBuffer.Bind();
    indirectDrawBuffer.Bind();
    if (boneBuffer.has_value()) {
        boneBuffer->BindBase(BONE_BUFFER_BINDING);
        boneOffsetBuffer->BindBase(BONE_OFFSET_BUFFER_BINDING);
    }
    
    //double start1 = Time();
    //glPointSize(4.0);
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, drawCount, 0); // TODO: GET INDIRECT DRAWING TO WORK
    unsigned int i = 0;
    for (auto & command: drawCommands) {
        i++;
        if (command.count == 1) {
            DebugLogError("BRUH");
        }
        
        if (command.count == 0) {continue;}
        //glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)((i - 1) * sizeof(IndirectDrawCommand)));
        //glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, command.count, GL_UNSIGNED_INT, (void*)(unsigned long long)(command.firstIndex * sizeof(GLuint)), command.instanceCount, command.baseVertex, command.baseInstance + (instancedVertexBuffer.GetOffset() / instancedVertexSize));
    }
    
    
}

void Meshpool::Commit() {
    // write indirect draw commands to buffer
    for (auto it = pendingDrawCommandUpdates.begin(); it != pendingDrawCommandUpdates.end();) {
        auto& [slot, update] = *it;
        assert(update.updatesLeft != 0);
        update.updatesLeft--;

        DebugLogInfo("SO ", instanceCapacity, " size ", instancedVertexSize);
        DebugLogInfo("Adding ", slot, ", current ", update.updatesLeft, " adding ", (indirectDrawBuffer.GetOffset() / indirectDrawBuffer.GetSize()));
        
        IndirectDrawCommand command = drawCommands[slot];
        DebugLogInfo("BI = ", command.baseInstance);
        command.baseInstance += instanceCapacity * (instancedVertexBuffer.GetOffset() / instancedVertexSize);
        memcpy(indirectDrawBuffer.Data() + (slot * sizeof(IndirectDrawCommand)), &command, sizeof(IndirectDrawCommand));

        if (update.updatesLeft == 0) {
            it = pendingDrawCommandUpdates.erase(it);
            DebugLogInfo("bye bye")
        }
        else {
            it++;
        }
    }

    vertexBuffer.Commit();
    instancedVertexBuffer.Commit();
    indexBuffer.Commit();
    indirectDrawBuffer.Commit();
    if (boneBuffer) {
        boneBuffer->Commit();
        boneOffsetBuffer->Commit();
    }
}

void Meshpool::FlipBuffers()
{
    vertexBuffer.Flip();
    instancedVertexBuffer.Flip();
    indexBuffer.Flip();
    indirectDrawBuffer.Flip();
    if (boneBuffer) {
        boneBuffer->Flip();
        boneOffsetBuffer->Flip();
    }
}

void Meshpool::ExpandNonInstanced() {
    // Increase cap
    unsigned int oldMeshCapacity = meshCapacity;
    meshCapacity += baseMeshCapacity;
    for (unsigned int i = 0; i < baseInstanceCapacity; i++) {
        availableMeshSlots[0].push_back(oldMeshCapacity + i);
    }
    drawCommands.resize(meshCapacity);

    // Resize buffers.
    vertexBuffer.Reallocate(meshCapacity * meshVerticesSize);
    indexBuffer.Reallocate(meshCapacity * meshIndicesSize);
    indirectDrawBuffer.Reallocate(meshCapacity * sizeof(IndirectDrawCommand));

    // Create vao
    GLuint newVao;
    glGenVertexArrays(1, &newVao);

    glBindVertexArray(newVao);
    vertexBuffer.Bind();
    vertexFormat.SetNonInstancedVaoVertexAttributes(newVao, instancedVertexSize, nonInstancedVertexSize);

    // because we just recreated the vao, we have to rebind the instanced attributes too 
    // but when we're initializing, we don't want to do this because calling ExpandInstanced() in initializiation will and we don't have an instanced vertex buffer yet
    if (instancedVertexBuffer.bufferId != 0) {
        instancedVertexBuffer.Bind();
        vertexFormat.SetInstancedVaoVertexAttributes(newVao, instancedVertexSize, nonInstancedVertexSize);
    }
    

    // Delete the old vao if there is one.
    if (oldMeshCapacity != 0) {
        glDeleteVertexArrays(1, &vao);
    }

    // lastly, save the new vao we created
    vao = newVao;
    
}

// void Meshpool::ExpandAnimationBuffers(GLuint multiplier) {
//     Assert(vertexFormat.supportsAnimation);
//     boneBuffer->Reallocate(boneBuffer->GetSize() + (multiplier * BONE_BUFFER_EXPANSION_SIZE));
//     boneOffsetBuffer->Reallocate(boneOffsetBuffer->GetSize() + (multiplier * BONE_OFFSET_BUFFER_EXPANSION_SIZE));
// }


void Meshpool::ExpandInstanced(GLuint multiplier) {
    // Increase cap
    //unsigned int oldInstanceCapacity = instanceCapacity;
    instanceCapacity += baseInstanceCapacity * multiplier;
    // for (unsigned int i = 0; i < baseInstanceCapacity; i++) {
    //     availableInstancedSlots.push_back(oldInstanceCapacity + i);
    // }

    // Update buffers.
    instancedVertexBuffer.Reallocate(instanceCapacity * instancedVertexSize);
    if (vertexFormat.supportsAnimation) {
        boneBuffer->Reallocate(instanceCapacity * sizeof(glm::mat4x4) * vertexFormat.maxBones);
        boneOffsetBuffer->Reallocate(instanceCapacity * sizeof(GLuint));
    }
    

    // Associate data with the vao and describe format of instanced data
    glBindVertexArray(vao);
    instancedVertexBuffer.Bind();
    vertexFormat.SetInstancedVaoVertexAttributes(vao, instancedVertexSize, nonInstancedVertexSize);
}

void Meshpool::FillSlot(const unsigned int meshId, const unsigned int slot, const unsigned int instanceCount, bool modifying) {
    if (modifying) {
        // std::cout << "OK BOYS WE ARE IN FACT MODIFYING\n";
    }

    auto mesh = Mesh::Get(meshId);
    auto vertices = &mesh->vertices;
    auto indices = &mesh->indices;

    // std::cout << "\tWe gonna copy for meshid " << meshId << "!\n";
    // literally just memcpy into the buffers
    // std::cout << "\tOk so dst " << (void*)vertexBuffer.Data() << " slot " << slot << " mVS " << meshVerticesSize << " src " << vertices->data() << " count " << vertices->size() << " size " << nonInstancedVertexSize << "\n";
    memcpy(vertexBuffer.Data() + (slot * meshVerticesSize), vertices->data(), vertices->size() * sizeof(GLfloat) /*vertexSize*/);
    // std::cout << "\tvertices done\n";
    memcpy(indexBuffer.Data() + (slot * meshIndicesSize), indices->data(), indices->size() * sizeof(GLuint));
    // std::cout << "\t...but what did it cost?\n";

    // also update drawCommands before doing the indirect draw buffer
    drawCommands[slot].count = indices->size();
    drawCommands[slot].firstIndex = (slot * meshIndicesSize)/sizeof(GLuint);
    drawCommands[slot].baseVertex = slot * (meshVerticesSize/nonInstancedVertexSize);
    if (modifying != true) {
        drawCommands[slot].baseInstance = (slot == 0) ? 0: slotToInstanceLocations[slot - 1] + slotInstanceReservedCounts[slot - 1];
        drawCommands[slot].instanceCount = instanceCount;

        // std::printf("\tOnce again we're printing this stuff; %u %u   %u %u %u %u %u\n", meshVerticesSize, nonInstancedVertexSize, drawCommands[slot].count, drawCommands[slot].firstIndex, drawCommands[slot].baseVertex, drawCommands[slot].baseInstance, drawCommands[slot].instanceCount);

        // idk what to call this
        slotToInstanceLocations[slot] = drawCommands[slot].baseInstance;
        slotInstanceReservedCounts[slot] = mesh->instanceCount;
        slotContents[meshId].push_back(slot);

        // make sure instanced data buffer has room
        if (drawCommands[slot].baseInstance + drawCommands[slot].instanceCount > instanceCapacity) { 
            auto missingSlots = (drawCommands[slot].baseInstance + drawCommands[slot].instanceCount) - instanceCapacity;
            auto multiplier = missingSlots/baseInstanceCapacity + (missingSlots % instanceCapacity != 0); // this is just integer division that rounds up (so we always expand by at least 1)
            ExpandInstanced(multiplier);
        }
    }
    

    
    UpdateIndirectDrawBuffer(slot);
}

void Meshpool::UpdateIndirectDrawBuffer(const unsigned int slot) { 
    pendingDrawCommandUpdates[slot] = IndirectDrawCommandUpdate{.updatesLeft = INSTANCED_VERTEX_BUFFERING_FACTOR};

    //memcpy(indirectDrawBuffer.Data() + (slot * sizeof(IndirectDrawCommand)), &drawCommands[slot], sizeof(IndirectDrawCommand));
}

int Meshpool::ScoreMeshFit(const unsigned int verticesNBytes, const unsigned int indicesNBytes, const MeshVertexFormat& meshVertexFormat) {
    if (vertexFormat != meshVertexFormat) { // make sure mesh compatible
        return -1;
    }
    if (verticesNBytes > meshVerticesSize || indicesNBytes > meshIndicesSize) { // if mesh too big forget it
        return -1;
    }
    else { 
        return std::min(meshVerticesSize - verticesNBytes, meshIndicesSize - indicesNBytes);
    }
}