// just a little struct used for the indirect drawing optimization in meshpool.cpp
// this struct's format is specifically required by OpenGL to be exactly ilke this
#pragma once
#include "GL/glew.h"
#include <utility/uint.hpp>

struct IndirectDrawCommand {
    CheckedUint count; // how many vertices to draw from index buffer
    CheckedUint instanceCount; // how many copies/instances to make of the object being drawn
    CheckedUint firstIndex; // starting location, in bytes, in index buffer
    GLint baseVertex; // number added to everything in index buffer, essential so that when we have two meshes in same buffer, their indices don't refer to the exact same vertices
    CheckedUint baseInstance; // number added to the instance id of each instance being drawn, effectively offsets position in the instanced data buffer.

    //IndirectDrawCommand() : 
    //count(0),
    //instanceCount(0),
    //firstIndex(0),
    //baseVertex(0),
    //baseInstance(0) {}

    std::string ToString() const;
};

struct IndirectDrawCommandUpdate {
    GLuint updatesLeft;
    
    // Here, command.baseInstance and command.baseVertex presume no multiple buffering. Meshpool::Commit() corrects that.
    IndirectDrawCommand command;

    // the index of the IndirectDrawCommand that needs to be updated (equivalent to the slot of the mesh this command handles)
    unsigned int commandSlot;
};