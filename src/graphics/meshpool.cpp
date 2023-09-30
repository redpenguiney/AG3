#pragma once
#include "../../external_headers/GLEW/glew.h"
#include "../../external_headers/GLM/glm.hpp"
#include "mesh.cpp"
#include "indirect_draw_command.cpp"
#include "../debug/debug.cpp"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <tuple>
#include <utility>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "buffered_buffer.cpp"

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
    const unsigned int meshVerticesSize; // The vertex data of meshes inside the pool can be smaller but no bigger than this (if they're way smaller they should still go in a new mesh pool)
    const unsigned int meshIndicesSize; // same as meshVertexSize but for the indices of a mesh
    const bool instanceColor;
    const bool instanceTextureZ;

    const size_t vertexSize; // the size of a single vertex. (not to be confused with meshVertexSize, the maximum combined size of a mesh's vertices allowed by the pool)
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
    inline static const GLuint NORMAL_ATTRIBUTE = 4;
    inline static const GLuint MODEL_MATRIX_ATTRIBUTE = 5;

    void ExpandNonInstanced();
    void ExpandInstanced(GLuint multiplier);

    void FillSlot(const unsigned int meshId, const unsigned int slot, const unsigned int instanceCount);
    void UpdateIndirectDrawBuffer(const unsigned int slot);
};

// constructor takes mesh reference to set variables, doesn't actually add the given mesh or anything
Meshpool::Meshpool(std::shared_ptr<Mesh>& mesh): 
    instanceSize(sizeof(glm::mat4x4) + ((mesh->instancedColor) ? sizeof(glm::vec4) : 0) + (mesh->instancedTextureZ ? sizeof(GLfloat) : 0)), // instances will be bigger if the mesh also wants color/texturez to be instanced
    meshVerticesSize(sizeof(GLfloat) * std::pow(2, 1 + (int)std::log2(mesh->vertices.size()))),
    meshIndicesSize(meshVerticesSize),
    instanceColor(mesh->instancedColor),
    instanceTextureZ(mesh->instancedTextureZ),
    vertexSize(sizeof(glm::vec3) + sizeof(glm::vec3) + sizeof(glm::vec2) + ((!mesh->instancedColor) ? sizeof(glm::vec4) : 0) + ((!mesh->instancedTextureZ) ? sizeof(GLfloat) : 0)), // sizeof(vertexPos) + sizeof(vertexNormals) + sizeof(textureXY) + sizeof(color if not instanced) + sizeof(textureZ if not instanced) 
    baseMeshCapacity((TARGET_VBO_SIZE/meshVerticesSize) + 1), // +1 just in case the base capacity was somehow 0
    baseInstanceCapacity((TARGET_VBO_SIZE/instanceSize) + 1),

    vertexBuffer(GL_ARRAY_BUFFER, 1, 0),
    instancedVertexBuffer(GL_ARRAY_BUFFER, 3, 0),
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
    std::vector<std::pair<unsigned int, unsigned int>> objLocations;
    unsigned int meshInstanceCapacity = Mesh::Get(meshId)->instanceCount;
    auto& contents = slotContents[meshId];
    
    // if any slots for this mesh have room for more instances, try to fill them up first
    for (unsigned int slot : contents) {

        // Check if RemoveObject() created any spaces to put new instances in
        if (slotInstanceSpaces.count(slot)) {
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
        if (availableMeshSlots.size() == 0) { // expand the meshpool if there isn't room for the new mesh
            ExpandNonInstanced();
        }

        // first try to get slot with matching instance capacity, failing that do a whole new slot
        unsigned int slot;
        if (availableMeshSlots.count(meshInstanceCapacity)) {
            slot = availableMeshSlots[meshInstanceCapacity].front();
            availableMeshSlots[meshInstanceCapacity].pop_front();
        }
        else {
            slot = availableMeshSlots[0].front();
            availableMeshSlots[0].pop_front();
        }

        unsigned int start = drawCommands[slot].instanceCount;
        FillSlot(meshId, slot, std::min(count, meshInstanceCapacity));
        drawCount += 1;

        // add positions to objLocations
        for (unsigned int i = start; i < start + std::min(count, meshInstanceCapacity); i++) {
            objLocations.push_back(std::make_pair(slot, i));
        }

        //drawCommands[slot].instanceCount = std::min(count, instanceCapacity);
        count -= std::min(count, meshInstanceCapacity);
    }
    
    return objLocations;
}

// Frees the given object from the meshpool, so something else can use that space.
void Meshpool::RemoveObject(const unsigned int slot, const unsigned int instanceId) {    
    // make sure instanceId is valid
    // TODO: check slot is valid
    //auto t = Time();
    assert(drawCommands[slot].instanceCount > instanceId);
   
   // if the instance is at the end of that slot we can just do this the easy way
    if (slotInstanceReservedCounts[slot] == instanceId + 1) {
        drawCommands.at(slot).instanceCount -= 1;

        // if the slot right before this one (and ones before it, given we don't remove ALL instances) are marked as empty, we should free them from being pointlessly drawn
        for (unsigned int instanceToRemove = instanceId - 1; instanceToRemove > 0 && slotInstanceSpaces.count(instanceToRemove); instanceToRemove--) {
            slotInstanceSpaces.erase(instanceToRemove);
            drawCommands[slot].instanceCount -= 1;
        }
    }
    else {
        // otherwise we have to just mark the instance as empty
        SetModelMatrix(slot, instanceId, glm::mat4x4(0)); // make it so it can't be drawn
        //auto t = Time();
        slotInstanceSpaces[slot].insert(instanceId); // tell AddObject that this instance can be overwritten
        //LogElapsed(t);
    }
    
   
    // if we removed all instances from the slot, mark the slot as empty and available for another mesh to use
    const unsigned int actualInstancesInSlot = drawCommands[slot].instanceCount - ((slotInstanceSpaces.count(slot)) ? slotInstanceSpaces[slot].size() : 0);
    if (actualInstancesInSlot == 0) {
        availableMeshSlots[slotInstanceReservedCounts[slot]].push_back(slot);
        drawCommands.at(slot).instanceCount = 0;
        drawCommands.at(slot).count = 0;
    }

    //LogElapsed(t);
}

// Makes the given instance the given color.
// Will abort if mesh uses per-vertex color instead of per-instance color.
void Meshpool::SetColor(const unsigned int slot, const unsigned int instanceId, const glm::vec4 rgba) {
    // make sure this instance slot hasn't been deleted
    if (slotInstanceSpaces.count(slot)) {
        assert(!slotInstanceSpaces[slot].count(instanceId));
    }

    assert(instanceColor == true);
    glm::vec4* colorLocation = (glm::vec4*)(sizeof(glm::mat4x4) + instancedVertexBuffer.Data() + ((slotToInstanceLocations[slot] + instanceId) * instanceSize));

    // make sure we don't segfault 
    assert((char*)colorLocation + sizeof(glm::vec4) <= instancedVertexBuffer.Data() + (instanceSize * instanceCapacity)); 
    assert((char*)colorLocation >= instancedVertexBuffer.Data());
    
    *colorLocation = rgba;
}

// Makes the given instance the given textureZ.
// Will abort if mesh uses per-vertex color instead of per-instance color.
void Meshpool::SetTextureZ(const unsigned int slot, const unsigned int instanceId, const float textureZ) {
    // make sure this instance slot hasn't been deleted
    if (slotInstanceSpaces.count(slot)) {
        assert(!slotInstanceSpaces[slot].count(instanceId));
    }

    assert(instanceTextureZ == true);
    float* textureZLocation = (float*)(sizeof(glm::mat4x4) + ((instanceColor) ? sizeof(glm::vec4) : 0) + instancedVertexBuffer.Data() + ((slotToInstanceLocations[slot] + instanceId) * instanceSize));

    // make sure we don't segfault 
    assert((char*)textureZLocation + sizeof(float) <= instancedVertexBuffer.Data() + (instanceSize * instanceCapacity)); 
    assert((char*)textureZLocation >= instancedVertexBuffer.Data());
    
    *textureZLocation = textureZ;
}

// Makes the given instance use the given model matrix.
void Meshpool::SetModelMatrix(const unsigned int slot, const unsigned int instanceId, const glm::mat4x4 matrix) {
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
    
    //std::printf("\nBRUH IT ELAPSED %fms", Time() - start1);
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

    std::printf("\nVertex size is %llu Instance size is %u", vertexSize, instanceSize);

    // Create vao
    GLuint newVao;
    glGenVertexArrays(1, &newVao);

    // Associate data with the vao and describe format of instanced data
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

    // color (rgba)
    if (!instanceColor) {
        glEnableVertexAttribArray(COLOR_ATTRIBUTE);
        glVertexAttribPointer(COLOR_ATTRIBUTE, 4, GL_FLOAT, false, vertexSize, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(glm::vec3)));
        glVertexAttribDivisor(COLOR_ATTRIBUTE, 0); // don't instance
    }

    // textureZ
    if (!instanceTextureZ) {
        glEnableVertexAttribArray(TEXTURE_Z_ATTRIBUTE);
        glVertexAttribPointer(TEXTURE_Z_ATTRIBUTE, 1, GL_FLOAT, false, vertexSize, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(glm::vec3) + ((!instanceColor) ? sizeof(glm::vec4) : 0)));
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

    glVertexAttribDivisor(MODEL_MATRIX_ATTRIBUTE, 1); // do instance
    glVertexAttribDivisor(MODEL_MATRIX_ATTRIBUTE+1, 1);
    glVertexAttribDivisor(MODEL_MATRIX_ATTRIBUTE+2, 1);
    glVertexAttribDivisor(MODEL_MATRIX_ATTRIBUTE+3, 1);

    // color (rgba)
    if (instanceColor) {
        glEnableVertexAttribArray(COLOR_ATTRIBUTE);
        glVertexAttribPointer(COLOR_ATTRIBUTE, 4, GL_FLOAT, false, instanceSize, (void*)sizeof(glm::mat4x4));
        glVertexAttribDivisor(COLOR_ATTRIBUTE, 1); // don't instance
    }

    // textureZ
    if (instanceTextureZ) {
        glEnableVertexAttribArray(TEXTURE_Z_ATTRIBUTE);
        glVertexAttribPointer(TEXTURE_Z_ATTRIBUTE, 2, GL_FLOAT, false, instanceSize, (void*)(sizeof(glm::mat4x4) + ((instanceColor) ? sizeof(glm::vec4) : 0)));
        glVertexAttribDivisor(TEXTURE_Z_ATTRIBUTE, 1); // don't instance
    }
}

// Fills the given slot with the given mesh's vertices and indices.
void Meshpool::FillSlot(const unsigned int meshId, const unsigned int slot, const unsigned int instanceCount) {
    std::printf("\nFillling slot %u with %u %u %u", slot, instanceCount, vertexSize, meshVerticesSize);

    auto mesh = Mesh::Get(meshId);
    auto vertices = &mesh->vertices;
    auto indices = &mesh->indices;

    // literally just memcpy into the buffers
    memcpy(vertexBuffer.Data() + (slot * meshVerticesSize), vertices->data(), vertices->size() * vertexSize);
    memcpy(indexBuffer.Data() + (slot * meshIndicesSize), indices->data(), indices->size() * sizeof(GLuint));

    // also update drawCommands before doing the indirect draw buffer
    drawCommands[slot].count = indices->size();
    drawCommands[slot].firstIndex = (slot * meshIndicesSize);
    drawCommands[slot].baseVertex = slot * (meshVerticesSize/vertexSize);
    drawCommands[slot].baseInstance = (slot == 0) ? 0: slotToInstanceLocations[slot - 1] + slotInstanceReservedCounts[slot - 1];
    drawCommands[slot].instanceCount = instanceCount;

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