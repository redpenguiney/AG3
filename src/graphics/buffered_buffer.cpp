#include "buffered_buffer.hpp"
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cwchar>

// bufferCount is no buffering (1), double buffering (2) or triple buffering (3).
BufferedBuffer::BufferedBuffer(GLenum bindingLocation, const unsigned int bufferCount, GLuint initalSize): 
bufferBindingLocation(bindingLocation),
numBuffers(bufferCount)
{
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
    glDeleteBuffers(1, &_bufferId);
}

// Must call every frame, or this entire class is literally useless.
// Call AFTER drawing please.
void BufferedBuffer::Update() {
    // Make the buffer section we just modified have a sync object so we won't write to it again until the GPU has finished using it.
    sync[currentBuffer] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    currentBuffer += 1;
    if (currentBuffer == numBuffers) {
        currentBuffer = 0;
    }

    // Wait for the GPU to be finished with the new section of the buffer (triple buffering means this shouldn't happen much).
    if (sync[currentBuffer] != 0) {
        //std::printf("\nSync status was %x", glClientWaitSync(sync[currentBuffer], GL_SYNC_FLUSH_COMMANDS_BIT, SYNC_TIMEOUT));
    }
}

// Recreates the buffer with the given size, and copies the buffer's old data into the new one.
// Doesn't touch vaos obviously so you still need to do reset that after reallocation.
// newSize must be >= size.
void BufferedBuffer::Reallocate(unsigned int newSize) {
    unsigned int oldSize = size;
    size = newSize;
    assert(newSize >= size);

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
    glBindBuffer(bufferBindingLocation, bufferId);
}

// returns pointer to buffer contents that you can write to
char* BufferedBuffer::Data() {
    return _bufferData + (size * currentBuffer);
}

// returns offset in bytes from start of buffer to start of active part of it.
unsigned int BufferedBuffer::GetOffset() {
    return size * currentBuffer;
}
