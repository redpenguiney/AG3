#include "debug/log.hpp"
#if defined(_MSC_VER)
#include <Windows.h> // i can't believe we have to include the whole windows header just to print something in visual studio.

int dbg_stream_for_cout::sync()
{

    auto temp = std::cout.rdbuf();
    std::cout.rdbuf(original_cout_buf);
    auto s = str();
    std::cout << s;
    std::cout.rdbuf(temp);

    OutputDebugStringA(s.c_str());


    str(std::string()); // Clear the string buffer
    return 0;
}

dbg_stream_for_cout::dbg_stream_for_cout()
{
}

dbg_stream_for_cout::~dbg_stream_for_cout() {
    sync(); 
}


void TestPrint() {
    OutputDebugStringA("OH NO");
}
#endif