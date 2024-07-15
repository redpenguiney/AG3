#pragma once
#include "GL/glew.h"

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

    BufferedBuffer(GLenum bindingLocation, const unsigned int bufferCount, GLuint initalSize);
    ~BufferedBuffer();
    void Update();
    void Reallocate(unsigned int newSize);
    void Bind();
    void BindBase(unsigned int index);
    char* Data();
    unsigned int GetOffset();
    unsigned int GetSize(); // returns size as if there was no multiple buffering

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