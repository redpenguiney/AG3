#pragma once
#include "GL/glew.h"

// TODO: setting to disable persistent buffers on a per-buffer basis

// A single/double/triple-buffered buffer wrapper used internally for graphics. Used to send data to/from the GPU
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

    // bufferCount is no buffering (1), double buffering (2) or triple buffering (3). or higher, i guess.
    BufferedBuffer(GLenum bindingLocation, const unsigned int bufferCount, GLuint initalSize);
    ~BufferedBuffer();

    // Recreates the buffer with the given size, and copies the buffer's old data into the new one.
    // Doesn't touch vaos obviously so you still need to reset that after reallocation.
    // newSize must be >= size.
    void Reallocate(unsigned int newSize);

    // Call after updating the buffer and before using/drawing with it on the GPU. 
    // Must call every frame if (and only if) you're updating every frame.
    // Creates the sync object.
    void Commit();

    // Call before writing to the buffer/after using/drawing with it on the GPU.
    // Must call every frame if (and only if) you're updating every frame.
    // Waits for write access (which should usually be available immediately if multiple buffering) and (if multiple buffers) changes the buffer to the next one.
    // Might yield if GPU isn't ready for us to write the data, so call at the last possible second.
    void Flip();

    void Bind();
    void BindBase(unsigned int index);
    char* Data();
    unsigned int GetOffset();
    unsigned int GetSize(); // returns size as if there was no multiple buffering

    private:
    // After we issue OpenGL commands using part of this buffer, we can't write to that part of that buffer until the command has finished.
    // Triple/double buffering means this usually won't be an issue, but just in case we have one sync object for each part of the buffer.
    // Array of numBuffers in length.
    GLsync* sync;

    BufferedBuffer(const BufferedBuffer&) = delete;
    
    const unsigned int numBuffers;
    const static inline unsigned int SYNC_TIMEOUT = 1000000000;

    // Size of 1/numBuffers the actual buffer size
    GLuint size;
    GLuint currentBuffer;

    GLuint _bufferId;
    char* _bufferData;


};