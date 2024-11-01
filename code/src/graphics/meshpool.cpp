#include "meshpool.hpp"
#include "GL/glew.h"
#include "mesh.hpp"

#include "shader_program.hpp"
#include "material.hpp"

#include <algorithm>
#include "../debug/assert.hpp"
#include "gengine.hpp"

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

std::vector<Meshpool::DrawHandle> Meshpool::AddObject(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material, const std::shared_ptr<ShaderProgram>& shader, CheckedUint count)
{
    //DebugLogInfo("Adding count ", count, " for meshid ", mesh->meshId);

    // find valid slot for mesh
    CheckedUint slot;
    {
        // see if this mesh is already in the pool
        if (meshUsers.contains(mesh->meshId)) {
            slot = meshUsers.at(mesh->meshId);
            
        }
        else {
            // for simplicity we like to assume indices and vertices are the same size in bytes and pad up the difference
            CheckedUint meshNBytes = std::max(mesh->vertices.size() * size_t(vertexSize), mesh->indices.size() * indexSize);

            CheckedUint powerOfTwo = vertexSize;
            CheckedUint exponent = 0;
            while (powerOfTwo < meshNBytes) {
                powerOfTwo *= 2;
                exponent++;
            }
            Assert(exponent < availableMeshSlots.size());

            // first, check availableMeshSlots
            if (availableMeshSlots.at(exponent).size() > 0) {
                slot = availableMeshSlots[exponent].back();
                availableMeshSlots[exponent].pop_back();
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

            meshUpdates.emplace_back(MeshUpdate{ MESH_BUFFERING_FACTOR, mesh, slot });
            
            meshUsers[mesh->meshId] = slot;
            meshSlotContents[slot] = MeshSlotUsageInfo{
                .meshId = unsigned(mesh->meshId),
                .sizeClass = exponent,
                .nUsers = 0
            };
            
        }
    }
    
    
    

    std::vector<DrawHandle> ret;
    CheckedUint nCreated = 0;
    while (nCreated < count) {
        CheckedUint nInstances, firstInstance;
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

        auto drawCommandBufferIndex = GetCommandBuffer(shader, material);
        DrawCommandBuffer& commandBuffer = drawCommands[drawCommandBufferIndex].value();

        // find slot for draw command
        CheckedUint drawCommandIndex = commandBuffer.GetNewDrawCommandSlot();

        //DebugLogInfo("Wrote id ", mesh->meshId, " to command ", drawCommandIndex);

        IndirectDrawCommand command = {
            .count = static_cast<CheckedUint>(mesh->indices.size()),
            .instanceCount = nInstances,
            //.firstIndex = slot.value * (indexSize), /// TODO MIGHT BE WRONG 
            .firstIndex = slot.value, // TODO how in the world does THIS work?!?!
            .baseVertex = static_cast<int>(slot),
            .baseInstance = firstInstance
        };

        if (mesh->dynamic) {
            commandBuffer.dynamicMeshCommandLocations[mesh->meshId].push_back(drawCommandIndex);
        }

        commandBuffer.clientCommands[drawCommandIndex] = command;
        //commandBuffer.drawCount++;

        // fortunately, the last added one will take precedence when multiple of these update the same command
        commandBuffer.commandUpdates.emplace_back(IndirectDrawCommandUpdate{
            .updatesLeft = INSTANCED_VERTEX_BUFFERING_FACTOR,
            .command = command,
            .commandSlot = drawCommandIndex
            });

        nCreated += nInstances;

        for (CheckedUint i = 0; i < nInstances; i++) {
            ret.emplace_back(DrawHandle{
                .meshIndex = (int)slot,
                .instanceSlot = (int)(firstInstance + i),
                .drawBufferIndex = (int)drawCommandBufferIndex
                });

            instanceSlotsToCommands[firstInstance + i] = CommandLocation{ .drawCommandIndex = drawCommandIndex };
        }

        meshSlotContents[slot].nUsers++;

    }
    
    Assert(meshSlotContents[slot].nUsers != 0);

    return ret;
}

void Meshpool::RemoveObject(const DrawHandle& handle)
{
    

    // something else can use this instance
    availableInstanceSlots.push_back(handle.instanceSlot);

    auto& drawBuffer = drawCommands[handle.drawBufferIndex].value();

    auto commandIndex = instanceSlotsToCommands[handle.instanceSlot].drawCommandIndex;
    CheckedUint originalNInstances = drawBuffer.clientCommands[commandIndex].instanceCount;
    IndirectDrawCommand emptyCommand(0, 0, 0, 0, 0);

    Assert(handle.instanceSlot >= drawBuffer.clientCommands[commandIndex].baseInstance);
    Assert(handle.instanceSlot < originalNInstances + drawBuffer.clientCommands[commandIndex].baseInstance);

    //DebugLogInfo("Removing object with instance ", handle.instanceSlot, " mesh index ", handle.meshIndex, " command buffer ", handle.drawBufferIndex, " command index ", commandIndex, " object's command has (before this call) ", originalNInstances);

    // to remove the object, we need to remove its draw command, or, if it's one of multiple instances being drawn in a single command, we have to split that command up.
    if ((unsigned int)originalNInstances == 1) {

        // mark the draw command slot as free
        drawBuffer.availableDrawCommandSlots.push_back(instanceSlotsToCommands[handle.instanceSlot].drawCommandIndex);
        //DebugLogInfo("Freed command index ", instanceSlotsToCommands[handle.instanceSlot].drawCommandIndex);

        drawBuffer.clientCommands[commandIndex] = emptyCommand;
        drawBuffer.commandUpdates.emplace_back(IndirectDrawCommandUpdate{
            .updatesLeft = INSTANCED_VERTEX_BUFFERING_FACTOR,
            .command = emptyCommand,
            .commandSlot = instanceSlotsToCommands[handle.instanceSlot].drawCommandIndex
        });

        unsigned int meshId = meshSlotContents.at(handle.meshIndex).meshId;
        if (Mesh::Get(meshId)->dynamic) {
            for (unsigned int i = 0; i < drawBuffer.dynamicMeshCommandLocations[meshId].size(); i++) {
                if (drawBuffer.dynamicMeshCommandLocations[meshId][i] == instanceSlotsToCommands[handle.instanceSlot].drawCommandIndex) {
                    drawBuffer.dynamicMeshCommandLocations[meshId][i] = drawBuffer.dynamicMeshCommandLocations[meshId].back();
                    drawBuffer.dynamicMeshCommandLocations[meshId].pop_back();
                    if (drawBuffer.dynamicMeshCommandLocations[meshId].size() == 0) {
                        drawBuffer.dynamicMeshCommandLocations.erase(meshId);
                    }
                    break;
                }

            }
            
        }

        //drawBuffer.drawCount--;

        //if (drawBuffer.drawCount == 0) { // then we just took out the last thing being drawn in this indirect draw buffer, delete it
            //DebugLogInfo("Hmm? ", handle.drawBufferIndex);
            //drawCommands[handle.drawBufferIndex] = std::nullopt;
        //}

        Assert(meshSlotContents.contains(handle.meshIndex));
        Assert(meshSlotContents.at(handle.meshIndex).nUsers > 0);
        if ((unsigned int)(--(meshSlotContents.at(handle.meshIndex).nUsers)) == 0) { // decrement count and if we just took out the last command using this mesh, then we should free up the mesh too
            //DebugLogInfo("Freed mesh index ", handle.meshIndex);
            availableMeshSlots[meshSlotContents.at(handle.meshIndex).sizeClass].push_back(handle.meshIndex);
            //meshSlotContents.erase(meshSlotContents.at(handle.meshIndex).meshId);

            if (GraphicsEngine::Get().dynamicMeshLocations.count(meshSlotContents.at(handle.meshIndex).meshId)) {
                GraphicsEngine::Get().dynamicMeshLocations.erase(meshSlotContents.at(handle.meshIndex).meshId);
            }

            //DebugLogInfo("Erassing meshid ", meshSlotContents[handle.meshIndex].meshId , " at mesh index ", handle.meshIndex);
            meshUsers.erase(meshSlotContents[handle.meshIndex].meshId); // TODO: could potentially lead to unneccesarily recopying mesh
            meshSlotContents.erase(handle.meshIndex);
        }
    }
    else {
        // then one instance out of multiple is being removed; we have to (potentially) split up the draw command

        CheckedUint firstIndex = commandIndex;

        IndirectDrawCommand firstHalf = drawBuffer.clientCommands[firstIndex];
        IndirectDrawCommand secondHalf = firstHalf;
        
        Assert(firstHalf.baseInstance <= handle.instanceSlot);

        firstHalf.instanceCount = handle.instanceSlot - firstHalf.baseInstance;

        if (firstHalf.instanceCount + 1 < (unsigned int)originalNInstances) {
            secondHalf.baseInstance = handle.instanceSlot + 1;
            secondHalf.instanceCount = originalNInstances - (firstHalf.instanceCount) - 1;
        }
        else {
            secondHalf.instanceCount = 0;
        }

        Assert(firstHalf.instanceCount + secondHalf.instanceCount == originalNInstances - 1);

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

        // if we need a second half, add it
        if (secondHalf.instanceCount != 0) {
            meshSlotContents.at(handle.meshIndex).nUsers++;

            // find slot for draw command
            CheckedUint secondIndex = drawBuffer.GetNewDrawCommandSlot();
             
            drawBuffer.clientCommands[secondIndex] = secondHalf;
            //drawCount++;

            // fortunately, the last added one will take precedence when multiple of these update the same command
            drawBuffer.commandUpdates.emplace_back(IndirectDrawCommandUpdate{
                .updatesLeft = INSTANCED_VERTEX_BUFFERING_FACTOR,
                .command = secondHalf,
                .commandSlot = secondIndex
            });
            

            // TODO this makes me sad because O(n)
            for (CheckedUint i = secondHalf.baseInstance; i < secondHalf.baseInstance + secondHalf.instanceCount; i++) {
                instanceSlotsToCommands[i].drawCommandIndex = secondIndex;
            }
        }
    }
}

//void Meshpool::SetNormalMatrix(const DrawHandle& handle, const glm::mat3x3& normal)
//{
//    SetInstancedVertexAttribute<glm::mat3x3>(handle, MeshVertexFormat::AttributeIndexFromAttributeName(format.NORMAL_MATRIX_ATTRIBUTE_NAME), normal);
//}
//
//void Meshpool::SetModelMatrix(const DrawHandle& handle, const glm::mat4x4& model)
//{
//    SetInstancedVertexAttribute<glm::mat4x4>(handle, MeshVertexFormat::AttributeIndexFromAttributeName(format.MODEL_MATRIX_ATTRIBUTE_NAME), model);
//}

void Meshpool::SetBoneState(const DrawHandle& handle, CheckedUint nBones, glm::mat4x4* offsets)
{
    Assert(format.supportsAnimation);
        
    Assert(format.maxBones <= nBones.value);
    glm::mat4x4* bonesLocation = (glm::mat4x4*)(bones->Data() + handle.instanceSlot * sizeof(glm::mat4x4));
    
    // make sure we don't segfault 
    Assert(handle.instanceSlot < currentInstanceCapacity);
    Assert(handle.meshIndex < currentVertexCapacity);
    Assert((char*)bonesLocation <= bones->Data() + (sizeof(glm::mat4x4) * currentInstanceCapacity));
    Assert((char*)bonesLocation >= bones->Data());
    
    // copy in bone transforms
    memcpy(bonesLocation, offsets, nBones.value * sizeof(glm::mat4x4));
}

void Meshpool::Draw(bool prePostProc) {
    glBindVertexArray(vaoId);
    indices.Bind();
    if (bones.has_value()) {
        bones->BindBase(BONE_BUFFER_BINDING);
        boneOffsetBuffer->BindBase(BONE_OFFSET_BUFFER_BINDING);
    }
    
    //double start1 = Time();
    //glPointSize(4.0);

    // We want to sort the draw commands by shader and then by material to reduce bindings which hurt perf.
    std::vector<DrawCommandBuffer*> sortedDrawCommands;
    for (auto& maybeBuffer : drawCommands) {
        if (maybeBuffer.has_value() && maybeBuffer->shader->ignorePostProc != prePostProc) {
            sortedDrawCommands.push_back(&*maybeBuffer);
        }
    }
    std::sort(sortedDrawCommands.begin(), sortedDrawCommands.end(), [](const DrawCommandBuffer* a, const DrawCommandBuffer* b) {
        if (a->shader->shaderProgramId == b->shader->shaderProgramId) {
            if (a->material == nullptr) {
                return true;
            }
            else if (b->material == nullptr) {
                return false;
            }
            return a->material->id > b->material->id;
        }
        else {
            return a->shader->shaderProgramId > b->shader->shaderProgramId;
        }
    });

    for (auto& command : sortedDrawCommands) {
        command->buffer.Bind();
        auto& shader = command->shader;
        shader->Use();

        shader->Uniform("vertexColorEnabled", format.attributes.color.has_value());

        if (shader->useClusteredLighting) {
            shader->Uniform("pointLightCount", GraphicsEngine::Get().pointLightCount);
            shader->Uniform("spotLightCount", GraphicsEngine::Get().spotLightCount);
            shader->Uniform("pointLightOffset", CheckedUint(GraphicsEngine::Get().pointLightDataBuffer.GetOffset() / sizeof(GraphicsEngine::PointLightInfo)));
            shader->Uniform("spotLightOffset", CheckedUint(GraphicsEngine::Get().spotLightDataBuffer.GetOffset() / sizeof(GraphicsEngine::SpotLightInfo)));
        }

        
        GraphicsEngine::Get().pointLightDataBuffer.BindBase(0);
        GraphicsEngine::Get().spotLightDataBuffer.BindBase(1);

        if (command->material == nullptr) { // if we aren't using a material
            Material::Unbind();

            shader->Uniform("specularMappingEnabled", false);
            shader->Uniform("fontMappingEnabled", false);
            shader->Uniform("normalMappingEnabled", false);
            shader->Uniform("parallaxMappingEnabled", false);
            shader->Uniform("colorMappingEnabled", false);
        }
        else {

            auto& material = command->material;
            material->Use(shader);

            // if (materialId == 4) {
            //     std::cout << "BINDING THING WITH FONTMAP.\n";
            // }

            shader->Uniform("specularMappingEnabled", material->Count(Texture::SpecularMap));
            shader->Uniform("fontMappingEnabled", material->Count(Texture::FontMap));
            shader->Uniform("normalMappingEnabled", material->Count(Texture::NormalMap));
            shader->Uniform("parallaxMappingEnabled", material->Count(Texture::DisplacementMap));
            shader->Uniform("colorMappingEnabled", material->Count(Texture::ColorMap));
        }

        glPointSize(15.0);
        //for (unsigned int i = 0; i < command->GetDrawCount(); i++) {
            //auto cmd = command->clientCommands.at(i);
            //glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, cmd.count, GL_UNSIGNED_INT, (const void*)(cmd.firstIndex * sizeof(GLuint)).value, cmd.instanceCount, cmd.baseVertex, cmd.baseInstance);
        //}
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)command->buffer.GetOffset(), command->GetDrawCount(), 0);
    }
     
    
}

void Meshpool::Commit() {
    // write vertex/index changes to buffer
    for (unsigned int i = 0; i < meshUpdates.size(); i++) {
        auto meshUpdate = meshUpdates[i];

        // copy vertices and indices
        // TODO: if something only modifies a portion of a mesh, we could optimize that by not memcpying everything
        memcpy(vertices.Data() + meshUpdate.meshIndex * vertexSize, meshUpdate.mesh->vertices.data(), meshUpdate.mesh->vertices.size() * sizeof(GLfloat));
        memcpy(indices.Data() + meshUpdate.meshIndex * indexSize, meshUpdate.mesh->indices.data(), meshUpdate.mesh->indices.size() * sizeof(GLuint));

        meshUpdate.updatesLeft--;
        if (meshUpdate.updatesLeft == 0) {     
            // pop erase isn't always acceptable;
            // we have to ensure that when multiple updates affect the same object, 
            //  the last update (closest to the end of the vector) stays at the end so it takes precedence
            // Order doesn't need to be preserved between updates affecting different objects, though. (TODO: can we exploit this to avoid using erase()?)
            //if (meshUpdates.back().meshIndex == meshUpdates[i].meshIndex) {
                meshUpdates.erase(meshUpdates.begin() + i);
            //}
            //else {
                //meshUpdates[i] = meshUpdates.back();
                //meshUpdates.pop_back();
            //}
            i--;
        }
    }
    // write indirect draw commands to buffer
    //DebugLogInfo("Offset of  ", this, " is ", instances.GetOffset() / instanceSize);
    for (auto& drawBuffer : drawCommands) {
        if (!drawBuffer.has_value()) { continue; }
        for (auto it = drawBuffer->commandUpdates.begin(); it != drawBuffer->commandUpdates.end(); ) {
            auto& update = *it;
            Assert(update.updatesLeft != 0);
            update.updatesLeft--;

            IndirectDrawCommand command = update.command; // deliberate copy

            // The tricky thing is, when either vertices or instances grow, these base instances/vertices need to get fixed too!
            // (TODO: why didn't i just use 3 seperate buffers for triple buffering like a sane person)
            // TODO: also GL_UNSYCRONIZED_BIT good?
            command.baseInstance += instances.GetOffset() / instanceSize;
            //DebugLogInfo("Offsetting base instance by ", instances.GetOffset() / instanceSize);
            command.baseVertex += vertices.GetOffset() / vertexSize;

            //DebugLogInfo("UPdating ", update.commandSlot);
            memcpy(drawBuffer->buffer.Data() + (update.commandSlot * sizeof(IndirectDrawCommand)), &command, sizeof(IndirectDrawCommand));

            if (update.updatesLeft == 0) {
                // pop erase isn't always acceptable;
                // we have to ensure that when multiple updates affect the same object, 
                //  the last update (closest to the end of the vector) stays at the end so it takes precedence
                // Order doesn't need to be preserved between updates affecting different objects, though. (TODO: can we exploit this to avoid using erase()?)
                //if (drawBuffer->commandUpdates.back().commandSlot == drawBuffer->commandUpdates[i].commandSlot) {
                    it = drawBuffer->commandUpdates.erase(it);
                    
                //}
                //else {
                    //drawBuffer->commandUpdates[i] = drawBuffer->commandUpdates.back();
                    //drawBuffer->commandUpdates.pop_back();
                //}

                //DebugLogInfo("bye bye")
            }
            else {
                it++;
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
    if (currentVertexCapacity == 0) {
        currentVertexCapacity = 1;
    }
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

    // Tragically, for every indirect draw command we have to update the 2nd and 3rd buffers' baseVertex since it was offset to correct for the OLD instance buffer's size.
    if (MESH_BUFFERING_FACTOR > 1) {
        for (auto& b : drawCommands) {
            if (!b.has_value()) { continue; }
            CheckedUint commandSlot = 0;
            for (auto& command : b->clientCommands) {
                b->commandUpdates.emplace_back(IndirectDrawCommandUpdate{
                    .updatesLeft = 3,
                    .command = command,
                    .commandSlot = commandSlot
                    });
                commandSlot++;
            }
        }
    }
    
}

void Meshpool::DrawCommandBuffer::ExpandDrawCommandCapacity()
{
    // update capacity
    CheckedUint oldCapacity = currentDrawCommandCapacity;
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
    for (CheckedUint i = oldCapacity; i < currentDrawCommandCapacity; i++) {
        availableDrawCommandSlots.push_back(i);
    }
    std::sort(availableDrawCommandSlots.begin(), availableDrawCommandSlots.end(), std::greater<CheckedUint>());
}

Meshpool::DrawCommandBuffer::DrawCommandBuffer(DrawCommandBuffer&& old) noexcept :
    shader(old.shader),
    material(old.material),
    buffer(std::move(old.buffer)),
    currentDrawCommandCapacity(old.currentDrawCommandCapacity),
    availableDrawCommandSlots(old.availableDrawCommandSlots),
    commandUpdates(old.commandUpdates),
    clientCommands(old.clientCommands)
{
    old.shader = nullptr;
    old.material = nullptr;
}

Meshpool::DrawCommandBuffer& Meshpool::DrawCommandBuffer::operator=(DrawCommandBuffer&& old) noexcept
{
    return *this;
}

void Meshpool::ExpandInstanceCapacity()
{
    if (currentInstanceCapacity > instanceEnd) {
        return;
    }

    // determine new instance capacity
    if (currentInstanceCapacity == 0) {
        currentInstanceCapacity = 1;
    }
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

    //DebugLogInfo("Updating instance capacity.");

    // Tragically, for every indirect draw command we have to update the 2nd and 3rd buffers' baseInstance since it was offset to correct for the OLD instance buffer's size.
    if (INSTANCED_VERTEX_BUFFERING_FACTOR > 1) {
        for (auto& b : drawCommands) {
            if (!b.has_value()) { continue; }
            //DebugLogInfo("Updating ", b->clientCommands.size(), " for instance buffer resize");

            CheckedUint commandSlot = 0;
            for (auto& command : b->clientCommands) {
                if (command.instanceCount == 0) { continue; }
                b->commandUpdates.emplace_back(IndirectDrawCommandUpdate{
                    .updatesLeft = 3,
                    .command = command,
                    .commandSlot = commandSlot
                    });
                commandSlot++;
            }
        }
    }
    
}

CheckedUint Meshpool::GetCommandBuffer(const std::shared_ptr<ShaderProgram>& shader, const std::shared_ptr<Material>& material)
{
    CheckedUint i = 0;
    for (auto& buffer : drawCommands) {
        if (buffer.has_value() && buffer->shader == shader && buffer->material == material) {
            return i;
        }
        i++;
    }

    BufferedBuffer b(GL_DRAW_INDIRECT_BUFFER, INSTANCED_VERTEX_BUFFERING_FACTOR, 0);
    std::optional<DrawCommandBuffer> oB(std::nullopt);
    oB.emplace(shader, material, std::move(b));

    if (availableDrawCommandBufferIndices.size()) {
        CheckedUint index = availableDrawCommandBufferIndices.back();
        availableDrawCommandBufferIndices.pop_back();
        drawCommands.emplace(drawCommands.begin() + index, std::move(oB));
        return index;
    }
    else {
        drawCommands.emplace_back(std::move(oB));
        return drawCommands.size() - 1;
    }
}

Meshpool::DrawCommandBuffer::DrawCommandBuffer(const std::shared_ptr<ShaderProgram>& s, const std::shared_ptr<Material>& m, BufferedBuffer&& b):
    shader(s),
    material(m),
    buffer(std::move(b))
{

}

CheckedUint Meshpool::DrawCommandBuffer::GetNewDrawCommandSlot()
{
    if (availableDrawCommandSlots.size() == 0) {
        ExpandDrawCommandCapacity();
    }
    unsigned drawCommandIndex = availableDrawCommandSlots.back();
    availableDrawCommandSlots.pop_back();
    return drawCommandIndex;
}

int Meshpool::DrawCommandBuffer::GetDrawCount()
{
    return currentDrawCommandCapacity;
}

template<typename AttributeType>
void Meshpool::SetInstancedVertexAttribute(const DrawHandle& handle, const CheckedUint attributeName, const AttributeType& value) {
    CheckedUint attributeIndex = MeshVertexFormat::AttributeIndexFromAttributeName(attributeName); // TODO: could sparsely populate vertexAttributes but with name instead of index?
    
    Assert(attributeIndex < MeshVertexFormat::N_ATTRIBUTES);
    Assert(format.vertexAttributes[attributeIndex]->instanced == true);

    AttributeType* attributeLocation = (AttributeType*)(format.vertexAttributes[attributeIndex]->offset + instances.Data() + (handle.instanceSlot * instanceSize));

    bool test = int(1) < currentInstanceCapacity;
    // make sure we don't segfault 
    Assert(handle.instanceSlot < currentInstanceCapacity);
    Assert((char*)attributeLocation <= instances.Data() + (instanceSize * currentInstanceCapacity));
    Assert((char*)attributeLocation >= instances.Data());
    *attributeLocation = value;
}

// explicit template instantiations
template void Meshpool::SetInstancedVertexAttribute<glm::mat4x4>(const DrawHandle& handle, const CheckedUint attributeName, const glm::mat4x4&);
template void Meshpool::SetInstancedVertexAttribute<glm::mat3x3>(const DrawHandle& handle, const CheckedUint attributeName, const glm::mat3x3&);
template void Meshpool::SetInstancedVertexAttribute<glm::vec4>(const DrawHandle& handle, const CheckedUint attributeName, const glm::vec4&);
template void Meshpool::SetInstancedVertexAttribute<glm::vec3>(const DrawHandle& handle, const CheckedUint attributeName, const glm::vec3&);
template void Meshpool::SetInstancedVertexAttribute<glm::vec2>(const DrawHandle& handle, const CheckedUint attributeName, const glm::vec2&);
template void Meshpool::SetInstancedVertexAttribute<float>(const DrawHandle& handle, const CheckedUint attributeName, const float&);