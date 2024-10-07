#include "buffered_buffer.hpp"
#include "debug/debug.hpp"
#include "debug/assert.hpp"
#include <cstddef>
#include <cstdio>
#include <cwchar>
#include <iostream>
#include "indirect_draw_command.hpp"

BufferedBuffer::BufferedBuffer(GLenum bindingLocation, const unsigned int bufferCount, GLuint initalSize): 
bufferBindingLocation(bindingLocation),
numBuffers(bufferCount)
{
    // Assert(initalSize != 0);
    currentBuffer = 0;
    size = 0;
    if (initalSize != 0) {
        Reallocate(initalSize);
    }  

    sync = new GLsync[numBuffers];
    for (unsigned int i = 0; i < numBuffers; i++) {
        sync[i] = 0;
    }
}

BufferedBuffer::~BufferedBuffer() {
    //glBindBuffer(bufferBindingLocation, 0);

    if (_bufferId != 0) { // if this buffer was leftover from a move, the destructor shouldn't do anything
        glDeleteBuffers(1, &_bufferId);
    }
    
    // TODO: delete sync?
}

void BufferedBuffer::Flip() {
    // Make the buffer section we just modified have a sync object so we won't write to it again until the GPU has finished using it.
    //sync[currentBuffer] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    currentBuffer += 1;
    if (currentBuffer == numBuffers) {
        currentBuffer = 0;
    }
}

void BufferedBuffer::Commit() {
    //if (bufferBindingLocation == GL_DRAW_INDIRECT_BUFFER) {
        //IndirectDrawCommand* ptr = (IndirectDrawCommand*)(void*)_bufferData;
    //}

    // if the buffer we're about to write to is currently in use by the GPU we have to wait (should not happen often when multibuffering)
    if (sync[currentBuffer] != 0) {
        auto syncStatus = glClientWaitSync(sync[currentBuffer], GL_SYNC_FLUSH_COMMANDS_BIT, SYNC_TIMEOUT);
        glDeleteSync(sync[currentBuffer]);
        sync[currentBuffer] = 0;
        //if (syncStatus != GL_ALREADY_SIGNALED) { // then we had to wait or it errored (0x911c or 37148 means it waited successfully)
            //DebugLogInfo("Sync status was ", syncStatus);
        //}

    }
}


BufferedBuffer::BufferedBuffer(BufferedBuffer&& old) noexcept:
    bufferBindingLocation(old.bufferBindingLocation),
    numBuffers(old.numBuffers),
    _bufferData(old.bufferData),
    currentBuffer(old.currentBuffer),
    sync(old.sync),
    size(old.size),
    _bufferId(old.bufferId)
{
    old._bufferId = 0;
    old._bufferData = nullptr;
    old.size = 0;
    old.currentBuffer = 0;
    old.sync = nullptr;
}

BufferedBuffer& BufferedBuffer::operator=(BufferedBuffer&& other) noexcept
{
    return *this;
}

void BufferedBuffer::Reallocate(unsigned int newSize) {
    Assert(newSize != 0);
    unsigned int oldSize = size;
    size = newSize;
    Assert(newSize >= size);

    // Create a new buffer with the desired size and get a pointer to its contents.
    GLuint newBufferId;
    glGenBuffers(1, &newBufferId);

    auto flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
#ifdef _DEBUG
    flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_READ_BIT;
#endif

    glBindBuffer(bufferBindingLocation, newBufferId);
    glBufferStorage(bufferBindingLocation, newSize * numBuffers, nullptr, flags);
    void* newBufferData = glMapBufferRange(bufferBindingLocation, 0, newSize * numBuffers, flags);

    // If there was a previous buffer, copy its data in and then delete it.
    if (oldSize != 0) {
        memcpy(newBufferData, _bufferData, oldSize * numBuffers);
        glDeleteBuffers(1, &_bufferId);
    }

    // save new buffer
    _bufferId = newBufferId;
    _bufferData = (char*)newBufferData;
}

void BufferedBuffer::Bind() {
    Assert(bufferId != 0);
    glBindBuffer(bufferBindingLocation, bufferId);
}

void BufferedBuffer::BindBase(unsigned int index) {
    glBindBufferBase(bufferBindingLocation, index, bufferId);
}

// returns pointer to buffer contents that you can write to
char* BufferedBuffer::Data() {
    return _bufferData + (size * currentBuffer);
}

// returns offset in bytes from start of buffer to start of active part of it.
unsigned int BufferedBuffer::GetOffset() {
    return size * currentBuffer;
}

unsigned int BufferedBuffer::GetSize() {
    return size;
}
