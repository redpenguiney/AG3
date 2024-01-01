#include "../../external_headers/GLEW/glew.h"
#include "../../external_headers/GLM/glm.hpp"
#include "../debug/debug.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include "meshpool.hpp"

// TODO: use GL_SHORT for indices in meshpools where there are fewer than 65536 indices (wait is that even possible)

// constructor takes mesh reference to set variables, doesn't actually add the given mesh or anything
Meshpool::Meshpool(std::shared_ptr<Mesh>& mesh): 
    instanceSize(sizeof(glm::mat4x4) + sizeof(glm::mat3x3) + ((mesh->instancedColor) ? sizeof(glm::vec4) : 0) + (mesh->instancedTextureZ ? sizeof(GLfloat) : 0)), // instances will be bigger if the mesh also wants color/texturez to be instanced
    vertexSize(mesh->vertexSize), 
    meshVerticesSize(((((int)std::pow(2, 1 + (int)std::log2(mesh->vertices.size() * sizeof(GLfloat)))) + vertexSize - 1)/vertexSize) * vertexSize), // makes meshVerticesSize a power of two rounded to the nearest multiple of vertexSize (must be multiple of vertexSize for OpenGL base vertex argument to work)
    meshIndicesSize(meshVerticesSize),
    instanceColor(mesh->instancedColor),
    instanceTextureZ(mesh->instancedTextureZ),
    baseMeshCapacity((TARGET_VBO_SIZE/meshVerticesSize) + 1), // +1 just in case the base capacity was somehow 0
    baseInstanceCapacity((TARGET_VBO_SIZE/instanceSize) + 1),

    vertexBuffer(GL_ARRAY_BUFFER, 1, 0),
    instancedVertexBuffer(GL_ARRAY_BUFFER, INSTANCED_VERTEX_BUFFERING_FACTOR, 0),
    indexBuffer(GL_ELEMENT_ARRAY_BUFFER, 1, 0),
    indirectDrawBuffer(GL_DRAW_INDIRECT_BUFFER, 1, 0)
{ 
    vao = 0;

    meshCapacity = 0;
    instanceCapacity = 0;
    drawCount = 0;

    ExpandNonInstanced();
    ExpandInstanced(1);
}

// Adds count identical meshes to pool, and returns a vector of pairs of (slot, instanceOffset) used to access the object.
std::vector<std::pair<unsigned int, unsigned int>> Meshpool::AddObject(const unsigned int meshId, unsigned int count) {
    std::printf("\tBe advised: adding %u of %u\n", count, meshId);
    std::vector<std::pair<unsigned int, unsigned int>> objLocations;
    unsigned int meshInstanceCapacity = Mesh::Get(meshId)->instanceCount;

    // if no value for this key, will be automatically created as an empty vector
    auto& contents = slotContents[meshId];
    
    // if any slots for this mesh have room for more instances, try to fill them up first
    for (unsigned int slot : contents) {
        std::cout << "\tChecking slot " << slot << ".\n";
        // Check if RemoveObject() created any spaces to put new instances in
        if (slotInstanceSpaces.count(slot)) {
            std::cout << "\tSlot instance spaces has room.\n";
            unsigned int len = slotInstanceSpaces[slot].size();
            for (unsigned int i = 0; i < std::min(len, count); i++) {
                auto instance = *(slotInstanceSpaces[slot].begin());
                objLocations.push_back(std::make_pair(slot, instance));
                slotInstanceSpaces[slot].erase(instance);
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
            std::cout << "\tSlot has room at end.\n";
            const unsigned int space = meshInstanceCapacity - storedCount;

            // add positions to objLocations
            for (unsigned int i = drawCommands[slot].instanceCount; i < drawCommands[slot].instanceCount + std::min(count, space); i++) {
                objLocations.push_back(std::make_pair(slot, i));
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
        std::cout << "\tInsufficient room after all that, creating slot.\n";
        if (availableMeshSlots.size() == 0) { // expand the meshpool if there isn't room for the new mesh
            std::cout << "\tExpanding...\n";
            ExpandNonInstanced();
            std::cout << "\tExpanded.\n";
        }
        std::cout << "\tChecking for slots with matching instance capacity.\n";
        // first try to get slot with matching instance capacity, failing that do a whole new slot
        unsigned int slot;
        if (availableMeshSlots.count(meshInstanceCapacity)) {
            std::cout << "\tFound one.\n";
            slot = availableMeshSlots[meshInstanceCapacity].front();
            availableMeshSlots[meshInstanceCapacity].pop_front();
        }
        else {
            std::cout << "\tDid not find one, reserving slot for this mesh.\n";
            slot = availableMeshSlots[0].front();
            availableMeshSlots[0].pop_front();
        }

        std::cout << "\tSlot obtained.\n";
        unsigned int start = drawCommands[slot].instanceCount;
        FillSlot(meshId, slot, std::min(count, meshInstanceCapacity));
        std::cout << "\tFilled slot.\n";
        drawCount += 1;

        // add positions to objLocations
        for (unsigned int i = start; i < start + std::min(count, meshInstanceCapacity); i++) {
            objLocations.push_back(std::make_pair(slot, i));
        }
        std::cout << "\tSet objLocations.\n";

        //drawCommands[slot].instanceCount = std::min(count, instanceCapacity);
        count -= std::min(count, meshInstanceCapacity);
    }
    
    return objLocations;
}

// Frees the given object from the meshpool, so something else can use that space.
void Meshpool::RemoveObject(const unsigned int slot, const unsigned int instanceId) {    
    // don't remove all these debug print statements, i have a feeling we'll need them again soon

    // make sure instanceId is valid
    // TODO: check slot is valid
    //auto t = Time();
    //std::printf("\nMy guy %u %u %u", drawCommands[slot].instanceCount, instanceId, slot);
    assert(drawCommands[slot].instanceCount > instanceId);
   
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

void Meshpool::SetColor(const unsigned int slot, const unsigned int instanceId, const glm::vec4& rgba) {
    // make sure this instance slot hasn't been deleted
    if (slotInstanceSpaces.count(slot)) {
        assert(!slotInstanceSpaces[slot].count(instanceId));
    }

    assert(instanceColor == true);
    glm::vec4* colorLocation = (glm::vec4*)(sizeof(glm::mat4x4) + sizeof(glm::mat3x3) + instancedVertexBuffer.Data() + ((slotToInstanceLocations[slot] + instanceId) * instanceSize));

    // make sure we don't segfault 
    assert((char*)colorLocation + sizeof(glm::vec4) <= instancedVertexBuffer.Data() + (instanceSize * instanceCapacity)); 
    assert((char*)colorLocation >= instancedVertexBuffer.Data());
    
    *colorLocation = rgba;
}

void Meshpool::SetTextureZ(const unsigned int slot, const unsigned int instanceId, const float textureZ) {
    // make sure this instance slot hasn't been deleted
    if (slotInstanceSpaces.count(slot)) {
        assert(!slotInstanceSpaces[slot].count(instanceId));
    }
    
    assert(instanceTextureZ == true);
    float* textureZLocation = (float*)(sizeof(glm::mat4x4) + sizeof(glm::mat3x3) + ((instanceColor) ? sizeof(glm::vec4) : 0) + instancedVertexBuffer.Data() + ((slotToInstanceLocations[slot] + instanceId) * instanceSize));

    // make sure we don't segfault 
    assert((char*)textureZLocation + sizeof(float) <= instancedVertexBuffer.Data() + (instanceSize * instanceCapacity)); 
    assert((char*)textureZLocation >= instancedVertexBuffer.Data());
    
    *textureZLocation = textureZ;
}

void Meshpool::SetModelMatrix(const unsigned int slot, const unsigned int instanceId, const glm::mat4x4& matrix) {
    // make sure this instance slot hasn't been deleted
    if (slotInstanceSpaces.count(slot)) {
        assert(!slotInstanceSpaces[slot].count(instanceId));
    }
    

    glm::mat4x4* modelMatrixLocation = (glm::mat4x4*)(instancedVertexBuffer.Data() + ((slotToInstanceLocations[slot] + instanceId) * instanceSize));

    // make sure we don't segfault 
    assert((char*)modelMatrixLocation <= instancedVertexBuffer.Data() + (instanceSize * instanceCapacity)); 
    assert((char*)modelMatrixLocation >= instancedVertexBuffer.Data());
    *modelMatrixLocation = matrix;
}

void Meshpool::SetNormalMatrix(const unsigned int slot, const unsigned int instanceId, const glm::mat3x3& normal) {
    // make sure this instance slot hasn't been deleted
    if (slotInstanceSpaces.count(slot)) {
        assert(!slotInstanceSpaces[slot].count(instanceId));
    }
    

    glm::mat3x3* normalMatrixLocation = (glm::mat3x3*)(sizeof(glm::mat4x4) + instancedVertexBuffer.Data() + ((slotToInstanceLocations[slot] + instanceId) * instanceSize));

    // make sure we don't segfault 
    assert(instanceId < instanceCapacity);
    assert(slot < meshCapacity);
    assert((char*)normalMatrixLocation <= instancedVertexBuffer.Data() + (instanceSize * instanceCapacity)); 
    assert((char*)normalMatrixLocation >= instancedVertexBuffer.Data());
    *normalMatrixLocation = normal;
}

void Meshpool::Draw() {
    glBindVertexArray(vao);
    indexBuffer.Bind();
    indirectDrawBuffer.Bind();
    //double start1 = Time();
   // glMultiDrawElementsIndirect(GL_POINTS, GL_UNSIGNED_INT, 0, drawCount, 0); TODO: GET INDIRECT DRAWING TO WORK
    for (auto & command: drawCommands) {
        if (command.count == 0) {continue;}
        glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, command.count, GL_UNSIGNED_INT, (void*)(unsigned long long)command.firstIndex, command.instanceCount, command.baseVertex, command.baseInstance + (instancedVertexBuffer.GetOffset()/instanceSize));
    }
    
    
}

// needed for BufferedBuffer's double/triple buffering, call every frame.
void Meshpool::Update() {
    vertexBuffer.Update();
    instancedVertexBuffer.Update();
    indexBuffer.Update();
    indirectDrawBuffer.Update();
}

// expands non-instanced buffers to accomodate baseMeshCapacity more unique meshes.
// also makes the vao
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

    // Associate data with the vao and describe format of non-instanced data
    glBindVertexArray(newVao);

    // vertex pos
    glEnableVertexAttribArray(POS_ATTRIBUTE);
    glVertexAttribPointer(POS_ATTRIBUTE, 3, GL_FLOAT, false, vertexSize, (void*)0);
    glVertexAttribDivisor(POS_ATTRIBUTE, 0); // don't instance

    // textureXY 
    glEnableVertexAttribArray(TEXTURE_XY_ATTRIBUTE);
    glVertexAttribPointer(TEXTURE_XY_ATTRIBUTE, 2, GL_FLOAT, false, vertexSize, (void*)sizeof(glm::vec3));
    glVertexAttribDivisor(TEXTURE_XY_ATTRIBUTE, 0); // don't instance

    // vertex normal
    glEnableVertexAttribArray(NORMAL_ATTRIBUTE);
    glVertexAttribPointer(NORMAL_ATTRIBUTE, 3, GL_FLOAT, false, vertexSize, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2)));
    glVertexAttribDivisor(NORMAL_ATTRIBUTE, 0); // don't instance

    // atangent vector
    glEnableVertexAttribArray(ATANGENT_ATTRIBUTE);
    glVertexAttribPointer(ATANGENT_ATTRIBUTE, 3, GL_FLOAT, false, vertexSize, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(glm::vec3)));
    glVertexAttribDivisor(ATANGENT_ATTRIBUTE, 0); // don't instance

    // color (rgba)
    if (!instanceColor) {
        glEnableVertexAttribArray(COLOR_ATTRIBUTE);
        glVertexAttribPointer(COLOR_ATTRIBUTE, 4, GL_FLOAT, false, vertexSize, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(glm::vec3) + sizeof(glm::vec3)));
        glVertexAttribDivisor(COLOR_ATTRIBUTE, 0); // don't instance
    }

    // textureZ
    if (!instanceTextureZ) {
        glEnableVertexAttribArray(TEXTURE_Z_ATTRIBUTE);
        glVertexAttribPointer(TEXTURE_Z_ATTRIBUTE, 1, GL_FLOAT, false, vertexSize, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(glm::vec3) + sizeof(glm::vec3) + ((!instanceColor) ? sizeof(glm::vec4) : 0)));
        glVertexAttribDivisor(TEXTURE_Z_ATTRIBUTE, 0); // don't instance
    }

    // Delete the old vao if there is one.
    if (oldMeshCapacity != 0) {
        glDeleteVertexArrays(1, &vao);
    }

    // lastly, save the new vao we created
    vao = newVao;
    
}

// VAO MUST BE GENERATED BEFORE THIS FUNCTION IS CALLED.
// expands instance buffer to accomodate multiplier * baseInstanceCapacity more instances, or creates the instance buffer if needed.
// this one needs a multiplier argument because if we add like 20000 instances and the baseInstanceCapacity is 5000, there wouldn't be enough room and it would segfault
void Meshpool::ExpandInstanced(GLuint multiplier) {
    // Increase cap
    //unsigned int oldInstanceCapacity = instanceCapacity;
    instanceCapacity += baseInstanceCapacity * multiplier;
    // for (unsigned int i = 0; i < baseInstanceCapacity; i++) {
    //     availableInstancedSlots.push_back(oldInstanceCapacity + i);
    // }

    // Update buffer.
    instancedVertexBuffer.Reallocate(instanceCapacity * instanceSize);

    // Associate data with the vao and describe format of instanced data
    glBindVertexArray(vao);

    // model matrix
    // each vertex attribute has to be no more than a vec4, so for a whole model matrix we do 4 attributes
    // TODO: we could definitely get away with just a 4x3 model matrix that is converted into 4x4 in the vertex shader
    glEnableVertexAttribArray(MODEL_MATRIX_ATTRIBUTE);
    glEnableVertexAttribArray(MODEL_MATRIX_ATTRIBUTE+1);
    glEnableVertexAttribArray(MODEL_MATRIX_ATTRIBUTE+2);
    glEnableVertexAttribArray(MODEL_MATRIX_ATTRIBUTE+3);

    glVertexAttribPointer(MODEL_MATRIX_ATTRIBUTE, 4, GL_FLOAT, false, instanceSize, (void*)0);
    glVertexAttribPointer(MODEL_MATRIX_ATTRIBUTE+1, 4, GL_FLOAT, false, instanceSize, (void*)(sizeof(glm::vec4) * 1));
    glVertexAttribPointer(MODEL_MATRIX_ATTRIBUTE+2, 4, GL_FLOAT, false, instanceSize, (void*)(sizeof(glm::vec4) * 2));
    glVertexAttribPointer(MODEL_MATRIX_ATTRIBUTE+3, 4, GL_FLOAT, false, instanceSize, (void*)(sizeof(glm::vec4) * 3));

    glVertexAttribDivisor(MODEL_MATRIX_ATTRIBUTE, 1); // tell openGL that this is an instanced vertex attribute (meaning per instance, not per vertex)
    glVertexAttribDivisor(MODEL_MATRIX_ATTRIBUTE+1, 1);
    glVertexAttribDivisor(MODEL_MATRIX_ATTRIBUTE+2, 1);
    glVertexAttribDivisor(MODEL_MATRIX_ATTRIBUTE+3, 1);

    // normal matrix
    // just 3x3 so only 3 attributes
    glEnableVertexAttribArray(NORMAL_MATRIX_ATTRIBUTE);
    glEnableVertexAttribArray(NORMAL_MATRIX_ATTRIBUTE+1);
    glEnableVertexAttribArray(NORMAL_MATRIX_ATTRIBUTE+2);

    glVertexAttribPointer(NORMAL_MATRIX_ATTRIBUTE, 4, GL_FLOAT, false, instanceSize, (void*)(sizeof(glm::mat4x4)));
    glVertexAttribPointer(NORMAL_MATRIX_ATTRIBUTE+1, 4, GL_FLOAT, false, instanceSize, (void*)(sizeof(glm::mat4x4) + sizeof(glm::vec3) * 1));
    glVertexAttribPointer(NORMAL_MATRIX_ATTRIBUTE+2, 4, GL_FLOAT, false, instanceSize, (void*)(sizeof(glm::mat4x4) + sizeof(glm::vec3) * 2));

    glVertexAttribDivisor(NORMAL_MATRIX_ATTRIBUTE, 1); // tell openGL that this is an instanced vertex attribute (meaning per instance, not per vertex)
    glVertexAttribDivisor(NORMAL_MATRIX_ATTRIBUTE+1, 1);
    glVertexAttribDivisor(NORMAL_MATRIX_ATTRIBUTE+2, 1);

    // color (rgba)
    if (instanceColor) {
        glEnableVertexAttribArray(COLOR_ATTRIBUTE);
        glVertexAttribPointer(COLOR_ATTRIBUTE, 4, GL_FLOAT, false, instanceSize, (void*)(sizeof(glm::mat4x4) + sizeof(glm::mat3x3)));
        glVertexAttribDivisor(COLOR_ATTRIBUTE, 1); // don't instance
    }

    // textureZ
    if (instanceTextureZ) {
        glEnableVertexAttribArray(TEXTURE_Z_ATTRIBUTE);
        glVertexAttribPointer(TEXTURE_Z_ATTRIBUTE, 2, GL_FLOAT, false, instanceSize, (void*)(sizeof(glm::mat4x4) + sizeof(glm::mat3x3) + ((instanceColor) ? sizeof(glm::vec4) : 0)));
        glVertexAttribDivisor(TEXTURE_Z_ATTRIBUTE, 1); // don't instance
    }
}

// Fills the given slot with the given mesh's vertices and indices.
void Meshpool::FillSlot(const unsigned int meshId, const unsigned int slot, const unsigned int instanceCount) {

    auto mesh = Mesh::Get(meshId);
    auto vertices = &mesh->vertices;
    auto indices = &mesh->indices;

    std::cout << "\tWe gonna copy!\n";
    // literally just memcpy into the buffers
    std::cout << "\tOk so dst " << (void*)vertexBuffer.Data() << " slot " << slot << " mVS " << meshVerticesSize << " src " << vertices->data() << " count " << vertices->size() << " size " << vertexSize << "\n";
    memcpy(vertexBuffer.Data() + (slot * meshVerticesSize), vertices->data(), vertices->size() * sizeof(GLfloat) /*vertexSize*/);
    std::cout << "\tvertices done\n";
    memcpy(indexBuffer.Data() + (slot * meshIndicesSize), indices->data(), indices->size() * sizeof(GLuint));
    std::cout << "\t...but what did it cost?\n";

    // also update drawCommands before doing the indirect draw buffer
    drawCommands[slot].count = indices->size();
    drawCommands[slot].firstIndex = (slot * meshIndicesSize);
    drawCommands[slot].baseVertex = slot * (meshVerticesSize/vertexSize);
    drawCommands[slot].baseInstance = (slot == 0) ? 0: slotToInstanceLocations[slot - 1] + slotInstanceReservedCounts[slot - 1];
    drawCommands[slot].instanceCount = instanceCount;

    std::printf("\tOnce again we're printing this stuff; %u %u   %u %u %u %u %u\n", meshVerticesSize, vertexSize, drawCommands[slot].count, drawCommands[slot].firstIndex, drawCommands[slot].baseVertex, drawCommands[slot].baseInstance, drawCommands[slot].instanceCount);

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
    UpdateIndirectDrawBuffer(slot);
}

void Meshpool::UpdateIndirectDrawBuffer(const unsigned int slot) {
    memcpy(indirectDrawBuffer.Data() + (slot * sizeof(IndirectDrawCommand)), &drawCommands[slot], sizeof(IndirectDrawCommand));
}

// We want meshes to fit snugly in the slots of their meshpool.
// Returns -1 if mesh is too big/incompatible w/ meshpool, otherwise lower number = better fit for meshpool.
// The engine will put meshes in the best fitting pool.
int Meshpool::ScoreMeshFit(const unsigned int verticesNBytes, const unsigned int indicesNBytes, const bool shouldInstanceColor, const bool shouldInstanceTextureZ) {
    if (shouldInstanceColor != instanceColor || shouldInstanceTextureZ != instanceTextureZ) { // make sure mesh compatible
        return -1;
    }
    if (verticesNBytes > meshVerticesSize || indicesNBytes > meshIndicesSize) { // if mesh too big forget it
        return -1;
    }
    else {
        return std::min(meshVerticesSize - verticesNBytes, meshIndicesSize - indicesNBytes);
    }
}