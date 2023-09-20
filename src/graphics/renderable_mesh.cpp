#pragma once
#include "../../external_headers/GLEW/glew.h"
#include "mesh.cpp"
#include <cassert>
#include <vector>

// Very small class that renders hardcoded stuff (the box of the skybox and the screen quad for postprocessing, specifically) without a meshpool.
// You probably are looking for meshpool, not this.
class RenderableMesh {
    public:
    

    RenderableMesh(std::shared_ptr<Mesh> mesh) {
        assert(mesh->instancedColor && mesh->instancedTextureZ); // don't make the vertices have this stuff please i don't want to add vertex attributes for that

        indexCount = mesh->indices.size();

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ibo);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->indices.size() * sizeof(GLuint), mesh->indices.data(), GL_STATIC_DRAW);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, mesh->vertices.size() * sizeof(GLfloat), mesh->vertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(0, 3, GL_FLOAT, false, 8 * sizeof(GLfloat), 0); // position
        glVertexAttribPointer(1, 2, GL_FLOAT, false, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat))); // texture
        glVertexAttribPointer(2, 3, GL_FLOAT, false, 8 * sizeof(GLfloat), (void*)(5 * sizeof(GLfloat))); // normal
    }

    // no copy or default constructing
    RenderableMesh(const RenderableMesh& other) = delete;
    RenderableMesh() = delete;

    ~RenderableMesh() {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ibo);    
    }

    void Draw() {
        glBindVertexArray(vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    }

    private:
    GLuint vbo;
    GLuint ibo;
    GLuint vao;
    unsigned int indexCount;

};