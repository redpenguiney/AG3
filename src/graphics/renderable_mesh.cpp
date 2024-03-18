#include "renderable_mesh.hpp"
#include <cassert>
#include <iostream>

RenderableMesh::RenderableMesh(std::shared_ptr<Mesh> mesh) {
    assert(mesh->dynamic == false);
    // assert(mesh->vertexFormat.attributes.color->instanced && mesh->vertexFormat.attributes.textureZ->instanced); // don't make the vertices have this stuff please i don't want to add vertex attributes for that
    std::cout << "nonInstanced vertex size = " << mesh->nonInstancedVertexSize << ".\n";
    indexCount = mesh->indices.size();

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ibo);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->indices.size() * sizeof(GLuint), mesh->indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh->vertices.size() * sizeof(GLfloat), mesh->vertices.data(), GL_STATIC_DRAW);
    mesh->vertexFormat.SetNonInstancedVaoVertexAttributes(vao, mesh->instancedVertexSize, mesh->nonInstancedVertexSize);
    // mesh->vertexFormat.SetInstancedVaoVertexAttributes(vao, mesh->instancedVertexSize, mesh->nonInstancedVertexSize);
}

RenderableMesh::~RenderableMesh() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ibo);    
}

void RenderableMesh::Draw() {
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
}