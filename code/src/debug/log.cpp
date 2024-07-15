#if defined(_MSC_VER)
#include "debug/log.hpp"
#include <Windows.h> // i can't believe we have to include the whole windows header just to print something in visual studio.

int dbg_stream_for_cout::sync()
{
    OutputDebugString(str().c_str());
    
    str(std::string()); // Clear the string buffer
    return 0;
}

dbg_stream_for_cout::~dbg_stream_for_cout() { 
    sync(); 
}

#endif

void TestPrint() {
    OutputDebugString("OH NO");
}