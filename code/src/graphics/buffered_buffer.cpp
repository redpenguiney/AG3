#include "buffered_buffer.hpp"
#include "debug/debug.hpp"
#include "debug/assert.hpp"
#include <cstddef>
#include <cstdio>
#include <cwchar>
#include <iostream>

// bufferCount is no buffering (1), double buffering (2) or triple buffering (3). or higher, i guess.
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
    glDeleteBuffers(1, &_bufferId);
}

// Must call every frame if you're updating every frame.
// Call AFTER drawing please. (todo: wait what why?)
void BufferedBuffer::Update() {
    // Make the buffer section we just modified have a sync object so we won't write to it again until the GPU has finished using it.
    sync[currentBuffer] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    currentBuffer += 1;
    if (currentBuffer == numBuffers) {
        currentBuffer = 0;
    }

    // Wait for the GPU to be finished with the new section of the buffer (triple buffering means this shouldn't happen much).
    if (sync[currentBuffer] != 0) {
        // TODO: WHY DID WE COMMENT OUT THE SYNC HERE?? THAT SEEMS KINDA IMPORTANT LOW KEY
        //std::printf("\nSync status was %x", glClientWaitSync(sync[currentBuffer], GL_SYNC_FLUSH_COMMANDS_BIT, SYNC_TIMEOUT));
    }
}

// Recreates the buffer with the given size, and copies the buffer's old data into the new one.
// Doesn't touch vaos obviously so you still need to reset that after reallocation.
// newSize must be >= size.
void BufferedBuffer::Reallocate(unsigned int newSize) {
    Assert(newSize != 0);
    unsigned int oldSize = size;
    size = newSize;
    Assert(newSize >= size);

    // Create a new buffer with the desired size and get a pointer to its contents.
    GLuint newBufferId;
    glGenBuffers(1, &newBufferId);

    glBindBuffer(bufferBindingLocation, newBufferId);
    glBufferStorage(bufferBindingLocation, newSize * numBuffers, nullptr, GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT);
    void* newBufferData = glMapBufferRange(bufferBindingLocation, 0, newSize * numBuffers, GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT);

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
