#include "../../external_headers/GLEW/glew.h"
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cwchar>

// TODO: setting to disable persistent buffers on a per-buffer basis

// A double/triple-buffered buffer used internally by Meshpool.
// With persistently mapped buffers, you still can't write to data being read by the GPU. (i got visual artifacts when i didn't care).
// Because they work asyncronously, this means you need to use double/triple buffering (triple to account for drivers).
// This does double/triple memory footprint, which is why there is a constant to control whether it does single/double/triple buffering.
// Sorry for the bad name.
class BufferedBuffer {
    public:
    const GLenum bufferBindingLocation;

    // lets stuff be read only without a getter
    const GLuint& bufferId = _bufferId;
    char* const& bufferData = _bufferData;

    // bufferCount is no buffering (1), double buffering (2) or triple buffering (3).
    BufferedBuffer(GLenum bindingLocation, const unsigned int bufferCount, GLuint initalSize): 
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

    ~BufferedBuffer() {
        glDeleteBuffers(1, &_bufferId);
    }

    // Must call every frame, or this entire class is literally useless.
    // Call AFTER drawing please.
    void Update() {
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
    void Reallocate(unsigned int newSize) {
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

    void Bind() {
        glBindBuffer(bufferBindingLocation, bufferId);
    }

    // returns pointer to buffer contents that you can write to
    char* Data() {
        return _bufferData + (size * currentBuffer);
    }

    // returns offset in bytes from start of buffer to start of active part of it.
    unsigned int GetOffset() {
        return size * currentBuffer;
    }

    private:
    // After we issue OpenGL commands using part of this buffer, we can't write to that part of that buffer until the command has finished.
    // Triple buffering means this usually won't be an issue, but just in case we have one sync object for each part of the buffer.
    GLsync* sync;

    BufferedBuffer(const BufferedBuffer&) = delete;
    
    const unsigned int numBuffers;
    const static inline unsigned int SYNC_TIMEOUT = 1000000;

    // Size of 1/numBuffers the actual buffer size
    GLuint size;
    GLuint currentBuffer;

    GLuint _bufferId;
    char* _bufferData;
};