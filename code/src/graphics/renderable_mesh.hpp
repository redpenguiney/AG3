#pragma once
#include <vector>
#include "mesh.hpp"
#include "GL/glew.h"
#include <memory>


// Very small class that renders hardcoded stuff (the box of the skybox and the screen quad for postprocessing, specifically) without a meshpool.
// You probably are looking for meshpool, not this.
class RenderableMesh {
    public:
    RenderableMesh(std::shared_ptr<Mesh> mesh);

    // no copy or default constructing
    RenderableMesh(const RenderableMesh& other) = delete;
    RenderableMesh() = delete;

    ~RenderableMesh();
    void Draw();

    private:
    GLuint vbo;
    GLuint ibo;
    GLuint vao;
    unsigned int indexCount;

};