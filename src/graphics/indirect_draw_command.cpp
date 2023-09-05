// just a little struct used for the indirect drawing optimization in meshpool.cpp
// this struct's format is specifically required by OpenGL to be exactly ilke this
#pragma once
#include "../../external_headers/GLEW/glew.h"

struct IndirectDrawCommand {
    GLuint count; // how many vertices to draw from index buffer
    GLuint instanceCount; // how many copies/instances to make of the object being drawn
    GLuint firstIndex; // starting location, in bytes, in index buffer
    GLuint baseVertex; // number added to everything in index buffer, essential so that when we have two meshes in same buffer, their indices don't refer to the exact same vertices
    GLuint baseInstance; // number added to the instance id of each instance being drawn, effectively offsets position in the instanced data buffer

    IndirectDrawCommand() : 
    count(0),
    instanceCount(0),
    firstIndex(0),
    baseVertex(0),
    baseInstance(0) {}
};