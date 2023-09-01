#pragma once
#include "../../external_headers/GLEW/glew.h"
#include<GL/GL.h>
#include<vector>
#include<atomic>

std::atomic<int> _lastMeshId = 0;

class Mesh {
    public:
        int Id;
        std::vector<GLfloat> vertices;
        std::vector<GLuint> indices;
        Mesh(std::vector<GLfloat> &verts, std::vector<GLuint> &indies) {
            vertices = verts;
            indices = indies;
            Id = _lastMeshId++;
        };
};