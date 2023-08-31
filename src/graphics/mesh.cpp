#pragma once
#include<GL/GL.h>
#include<vector>

int _lastMeshId = 0;

class Mesh {
    public:
        int Id;
        std::vector<GLfloat> Vertices;
        std::vector<GLuint> Indices;
        Mesh(std::vector<GLfloat> &vertices, std::vector<GLuint> &indices) {
            Vertices = vertices;
            Indices = indices;
        };
};