#pragma once
#include "../../external_headers/GLEW/glew.h"
#include "../../external_headers/GLM/glm.hpp"
#include "mesh.cpp"
#include "indirect_draw_command.cpp"
#include <deque>
#include <vector>
#include <unordered_map>

// Contains an arbitrary number of arbitary meshes and is used to render them very quickly.
class Meshpool {
    public:
    // public bc engine needs this info to decide which pool to add a mesh to, or if it should create a new pool altogether
    const unsigned int instanceSize; // Each gameobject has one instance containing its data (at minimum a model matrix)
    const unsigned int meshVerticesSize; // The vertex data of meshes inside the pool can be smaller but no bigger than this (if they're way smaller they should still go in a new mesh pool)
    const unsigned int meshIndicesSize; // same as meshVertexSize but for the indices of a mesh
    const bool instanceColor;
    const bool instanceTextureZ;

    Meshpool(std::shared_ptr<Mesh>& mesh);
    ~Meshpool();
    std::vector<std::pair<unsigned int, unsigned int>> AddObject(const unsigned int meshId, unsigned int count);
    void Draw();

    int ScoreMeshFit(const unsigned int verticesNBytes, const unsigned int indicesNBytes, const bool shouldInstanceColor, const bool shouldInstanceTextureZ);

    private:
    const size_t vertexSize; // the size of a single vertex. (not to be confused with meshVertexSize, the maximum combined size of a mesh's vertices allowed by the pool)
    const unsigned int baseMeshCapacity; // everytime the mesh pool expands its non-instanced vertex buffer, it will add room for this many meshes (or a multiple of this number if more than baseMeshCapacity meshes were added at once)
    const unsigned int baseInstanceCapacity; // same as baseMeshCapacity but for the instanced vertex buffer
    unsigned int meshCapacity; // how many different meshes the pool can store
    unsigned int instanceCapacity; // how many instances the pool can store (every gameobject being rendered needs exactly one instance with position and what not, but they can share meshes)

    unsigned int drawCount;

    GLuint vao; // tells opengl how vertices are formatted

    GLuint vertexBuffer; // holds per-vertex data (like normals)
    GLuint instancedVertexBuffer; // holds per-instance/per-object data (like model matrix)
    GLuint indexBuffer; // holds mesh indices
    GLuint indirectDrawBuffer; // stores rendering commands, used for an optimization called indirect drawing

    // we call glMapBuffer to get a pointer to the contents of each buffer that we can freely write to. 
    // (it's also persistently mapped and coherent so we don't need to ever call glUnmapBuffer or sync anything).
    // (char* instead of void* so we can do pointer arithmetic)
    char* vertexBufferData; 
    char* instancedVertexBufferData; 
    char* indexBufferData; 
    char* indirectDrawBufferData; 

    std::deque<unsigned int> availableMeshSlots;
    //std::deque<unsigned int> availableInstancedSlots;
    std::unordered_map<unsigned int, std::vector<unsigned int>> slotContents; // key is meshId, value is a vector of indices/slots in the vertexBuffer containing this mesh (0 for first mesh, 1 for second, etc.)
                                                                      // needed for instancing

    std::unordered_map<unsigned int, unsigned int> slotInstanceCounts; // key is slot, value is number of instances reserved by that slot 

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

    void DeleteNonInstancedBuffers(); 
    void DeleteInstancedBuffer();

    void ExpandNonInstanced();
    void ExpandInstanced(GLuint multiplier);

    void FillSlot(const unsigned int meshId, const unsigned int slot, const unsigned int instanceCount);
    void UpdateIndirectDrawBuffer(const unsigned int slot);
};

// constructor takes mesh reference to set variables, doesn't actually add the given mesh or anything
Meshpool::Meshpool(std::shared_ptr<Mesh>& mesh): 
    instanceSize(sizeof(glm::mat4x4) + (mesh->instancedColor ? sizeof(glm::vec4) : 0) + (mesh->instancedTextureZ ? sizeof(GLfloat) : 0)), // instances will be bigger if the mesh also wants color/texturez to be instanced
    meshVerticesSize(std::pow(2, std::log2(mesh->vertices.size() - 1)) + 1),
    meshIndicesSize(meshVerticesSize),
    instanceColor(mesh->instancedColor),
    instanceTextureZ(mesh->instancedTextureZ),
    vertexSize(sizeof(glm::vec3) + sizeof(glm::vec3) + sizeof(glm::vec2) + (!mesh->instancedColor ? sizeof(glm::vec4) : 0) + (!mesh->instancedTextureZ ? sizeof(GLfloat) : 0)), // sizeof(vertexPos) + sizeof(vertexNormals) + sizeof(textureXY) + sizeof(color if not instanced) + sizeof(textureZ if not instanced) 
    baseMeshCapacity((TARGET_VBO_SIZE/meshVerticesSize) + 1), // +1 just in case the base capacity was somehow 0
    baseInstanceCapacity((TARGET_VBO_SIZE/instanceSize) + 1)
{
    vao = 0;
    vertexBuffer = 0;
    instancedVertexBuffer = 0;
    indexBuffer = 0;
    indirectDrawBuffer = 0;

    ExpandNonInstanced();
    ExpandInstanced(1);
}

Meshpool::~Meshpool() {
    DeleteNonInstancedBuffers();
    DeleteInstancedBuffer();
}

// Adds count identical meshes to pool, and returns a vector of pairs of (slot, instanceOffset) used to access the object.
std::vector<std::pair<unsigned int, unsigned int>> Meshpool::AddObject(const unsigned int meshId, unsigned int count) {
    std::vector<std::pair<unsigned int, unsigned int>> objLocations;
    unsigned int instanceCapacity = Mesh::Get(meshId)->instanceCount;
    auto& contents = slotContents[meshId];
    
    // if any slots for this mesh have room for more instances, try to fill them up first
    for (unsigned int slot : contents) {
        unsigned int storedCount = drawCommands[slot].count;
        if (storedCount < instanceCapacity) { // if this slot has room
            const unsigned int space = instanceCapacity - storedCount;
            
            // add positions to objLocations
            for (unsigned int i = drawCommands[slot].instanceCount; i < drawCommands[slot].instanceCount + std::min(count, space); i++) {
                objLocations.push_back(std::make_pair(slot, i));
            }

            drawCommands[slot].instanceCount += std::min(count, space);
            count -= std::min(count, space);

            // if we added all of the requested objects then we're done here
            if (count > 0) {
                return objLocations;
            }

        }
    }

    // failing that, create slots and fill them until count is 0
    while (count > 0) {
        if (availableMeshSlots.size() == 0) { // expand the meshpool if there isn't room for the new mesh
            ExpandNonInstanced();
        }
        unsigned int slot = availableMeshSlots.front();
        availableMeshSlots.pop_front();
        FillSlot(meshId, slot, std::min(count, instanceCapacity));

        // add positions to objLocations
        for (unsigned int i = drawCommands[slot].instanceCount; i < drawCommands[slot].instanceCount + std::min(count, instanceCapacity); i++) {
            objLocations.push_back(std::make_pair(slot, i));
        }

        drawCommands[slot].instanceCount = std::min(count, instanceCapacity);
        count -= std::min(count, instanceCapacity);
    }
    
    return objLocations;
}

void Meshpool::Draw() {
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectDrawBuffer);
    glMultiDrawElementsIndirect(GL_POINTS, GL_UNSIGNED_INT, 0, drawCommands.size(), 0);
}

// assumes buffers actually exist or will crash.
// Also deletes the vao.
void Meshpool::DeleteNonInstancedBuffers() {
    GLuint buffers[] = {vertexBuffer, indexBuffer, indirectDrawBuffer};
    glDeleteBuffers(3, buffers);
    glDeleteVertexArrays(1, &vao);
}

// assumes buffer actually exists or will crash
void Meshpool::DeleteInstancedBuffer() {
    glDeleteBuffers(1, &instancedVertexBuffer);
}

// expands non-instanced buffers to accomodate baseMeshCapacity more unique meshes.
// also makes the vao
void Meshpool::ExpandNonInstanced() {
    // Increase cap
    unsigned int oldMeshCapacity = meshCapacity;
    meshCapacity += baseMeshCapacity;
    for (unsigned int i = 0; i < baseInstanceCapacity; i++) {
        availableMeshSlots.push_back(oldMeshCapacity + i);
    }
    drawCommands.resize(meshCapacity);


    // We have to recreate the buffer to resize it because it's persistently mapped
    // Generate new buffers and allocate storage for them
    GLuint newVertexBuffer, newIndexBuffer, newIndirectDrawBuffer;
    GLuint buffers[] = {newVertexBuffer, newIndexBuffer, newIndirectDrawBuffer};
    glGenBuffers(3, buffers);
    glBindBuffer(GL_ARRAY_BUFFER, newVertexBuffer);
    glBufferStorage(GL_ARRAY_BUFFER, meshVerticesSize * meshCapacity, nullptr, GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, newIndexBuffer);
    glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, meshIndicesSize * meshCapacity, nullptr, GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, newVertexBuffer);
    glBufferStorage(GL_DRAW_INDIRECT_BUFFER, sizeof(IndirectDrawCommand) * meshCapacity, nullptr, GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT);

    // Get pointer to buffer contents
    void* newVertexBufferData = glMapBuffer(GL_ARRAY_BUFFER, GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT);
    void* newIndexBufferData = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT);
    void* newIndirectDrawBufferData = glMapBuffer(GL_DRAW_INDIRECT_BUFFER, GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT);

    // Create vao
    GLuint newVao;
    glGenVertexArrays(1, &newVao);

    // Associate data with the vao and describe format of instanced data
    glBindVertexArray(vao);

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
        glVertexAttribPointer(TEXTURE_Z_ATTRIBUTE, 1, GL_FLOAT, false, vertexSize, (void*)(sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(glm::vec3) + (!instanceColor ? sizeof(glm::vec4) : 0)));
        glVertexAttribDivisor(TEXTURE_Z_ATTRIBUTE, 0); // don't instance
    }

    // If we had a previous instancedVertexBuffer, we need to memcpy the contents of the old buffer into the new one, then delete the old ones
    if (oldMeshCapacity != 0) {
        memcpy(newVertexBufferData, vertexBufferData, oldMeshCapacity * meshVerticesSize);
        memcpy(newIndexBufferData, indexBufferData, oldMeshCapacity * meshIndicesSize);
        memcpy(newIndirectDrawBufferData, indirectDrawBufferData, oldMeshCapacity * sizeof(IndirectDrawCommand));
        DeleteNonInstancedBuffers();
    }

    // lastly, save the new buffers and vao we created
    vertexBuffer = newVertexBuffer;
    vertexBufferData = (char*)newVertexBufferData;
    indexBuffer = newIndexBuffer;
    indexBufferData = (char*)newIndexBufferData;
    indirectDrawBuffer = newIndirectDrawBuffer;
    indirectDrawBufferData = (char*)newIndirectDrawBufferData;
    vao = newVao;
}

// VAO MUST BE GENERATED BEFORE THIS FUNCTION IS CALLED.
// expands instance buffer to accomodate multiplier * baseInstanceCapacity more instances, or creates the instance buffer if needed.
// this one needs a multiplier argument because if we add like 20000 instances and the baseInstanceCapacity is 5000, there wouldn't be enough room and it would segfault
void Meshpool::ExpandInstanced(GLuint multiplier) {
    // Increase cap
    unsigned int oldInstanceCapacity = instanceCapacity;
    instanceCapacity += baseInstanceCapacity * multiplier;
    // for (unsigned int i = 0; i < baseInstanceCapacity; i++) {
    //     availableInstancedSlots.push_back(oldInstanceCapacity + i);
    // }

    // We have to recreate the buffer to resize it because it's persistently mapped
    // Generate new buffer and allocate storage for it
    GLuint newInstancedVertexBuffer;
    glGenBuffers(1, &newInstancedVertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, newInstancedVertexBuffer);
    glBufferStorage(GL_ARRAY_BUFFER, instanceSize * instanceCapacity, nullptr, GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT);

    // Get pointer to buffer contents
    void* newInstancedVertexBufferData = glMapBuffer(GL_ARRAY_BUFFER, GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT);

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
    glVertexAttribPointer(MODEL_MATRIX_ATTRIBUTE+1, 4, GL_FLOAT, false, instanceSize, (void*)(sizeof(glm::vec4) * 2));
    glVertexAttribPointer(MODEL_MATRIX_ATTRIBUTE+2, 4, GL_FLOAT, false, instanceSize, (void*)(sizeof(glm::vec4) * 3));
    glVertexAttribPointer(MODEL_MATRIX_ATTRIBUTE+3, 4, GL_FLOAT, false, instanceSize, (void*)(sizeof(glm::vec4) * 4));

    glVertexAttribDivisor(MODEL_MATRIX_ATTRIBUTE, 1); // do instance
    glVertexAttribDivisor(MODEL_MATRIX_ATTRIBUTE+1, 1);
    glVertexAttribDivisor(MODEL_MATRIX_ATTRIBUTE+2, 1);
    glVertexAttribDivisor(MODEL_MATRIX_ATTRIBUTE+3, 1);

    // color (rgba)
    if (instanceColor) {
        glEnableVertexAttribArray(COLOR_ATTRIBUTE);
        glVertexAttribPointer(COLOR_ATTRIBUTE, 5, GL_FLOAT, false, instanceSize, (void*)sizeof(glm::mat4x4));
        glVertexAttribDivisor(COLOR_ATTRIBUTE, 1); // don't instance
    }

    // textureZ
    if (instanceTextureZ) {
        glEnableVertexAttribArray(TEXTURE_Z_ATTRIBUTE);
        glVertexAttribPointer(TEXTURE_Z_ATTRIBUTE, 2, GL_FLOAT, false, instanceSize, (void*)(sizeof(glm::mat4x4) + ((instanceColor) ? sizeof(glm::vec4) : 0)));
        glVertexAttribDivisor(TEXTURE_Z_ATTRIBUTE, 1); // don't instance
    }

    // If we had a previous instancedVertexBuffer, we need to memcpy the contents of the old buffer into the new one, then delete the old ones
    if (oldInstanceCapacity != 0) {
        memcpy(newInstancedVertexBufferData, instancedVertexBufferData, oldInstanceCapacity * instanceSize);
        DeleteInstancedBuffer();
    }

    // lastly, save the new buffers we created
    instancedVertexBuffer = newInstancedVertexBuffer;
    instancedVertexBufferData = (char*)newInstancedVertexBufferData;
}

// Fills the given slot with the given mesh's vertices and indices.
void Meshpool::FillSlot(const unsigned int meshId, const unsigned int slot, const unsigned int instanceCount) {
    auto mesh = Mesh::Get(meshId);
    auto vertices = &mesh->vertices;
    auto indices = &mesh->indices;

    // literally just memcpy into the buffers
    memcpy(vertexBufferData + (slot * meshVerticesSize), vertices->data(), vertices->size() * vertexSize);
    memcpy(indexBufferData + (slot * meshIndicesSize), indices->data(), indices->size() * sizeof(GLuint));

    // also update drawCommands before doing the indirect draw buffer
    drawCommands[slot].count = indices->size();
    drawCommands[slot].firstIndex = (slot * meshIndicesSize);
    drawCommands[slot].baseVertex = slot * (meshVerticesSize/vertexSize);
    drawCommands[slot].baseInstance = (slot == 0) ? 0: slotToInstanceLocations[slot - 1] + slotInstanceCounts[slot - 1];
    drawCommands[slot].instanceCount = instanceCount;

    // make sure instanced data buffer has room
    if (drawCommands[slot].baseInstance + drawCommands[slot].instanceCount > instanceCapacity) { 
        auto missingSlots = (drawCommands[slot].baseInstance + drawCommands[slot].instanceCount) - instanceCapacity;
        auto multiplier = missingSlots/baseInstanceCapacity + (missingSlots % instanceCapacity != 0); // this is just integer division that rounds up (so we always expand by at least 1)
        ExpandInstanced(multiplier);
    }
    UpdateIndirectDrawBuffer(slot);
}

void Meshpool::UpdateIndirectDrawBuffer(const unsigned int slot) {
    memcpy(indirectDrawBufferData + (slot * sizeof(IndirectDrawCommand)), (char*)drawCommands.data() + (slot * sizeof(IndirectDrawCommand)), sizeof(IndirectDrawCommand));
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