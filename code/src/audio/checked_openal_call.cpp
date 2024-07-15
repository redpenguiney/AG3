// OpenAL doesn't do nice error checking like OpenGL does so we have to call a get error function after every openAL call, which is what this does.

#pragma once
#include "AL/al.h"
#include "AL/alc.h"
#include <cstdlib>
#include <iostream>
#include <string>

#define CHECK_FOR_AL_ERRORS

#ifdef CHECK_FOR_AL_ERRORS

inline void CheckOpenALError(const char* fname, int line)
{
    ALenum err = alGetError();
    if (err != AL_NO_ERROR)
    {
        std::string errorName;
        switch (err) {
        case AL_INVALID_ENUM: 
        errorName = "AL_INVALID_ENUM";
        break;
        case AL_INVALID_VALUE: 
        errorName = "AL_INVALID_VALUE";
        break;
        case AL_INVALID_OPERATION: 
        errorName = "AL_INVALID_OPERATION";
        break;
        case AL_OUT_OF_MEMORY: 
        errorName = "AL_OUT_OF_MEMORY";
        break;
        /* ... */
        default:
            errorName = "with unrecognized code " + std::to_string(err);
        }

        std::cout << "Fatal OpenAL error " << errorName << " at line " << line << " of " << fname << ". Aborting.\n";
        abort();
    }
}

#define CheckedOpenALCall(stmt) { \
    stmt; \
    CheckOpenALError(__FILE__, __LINE__); \
}
#else
#define CheckedOpenAlCall(stmt) { \
    stmt; \
}
#endif