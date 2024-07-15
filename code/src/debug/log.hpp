#pragma once
#include <iostream>

// TODO: line numbers

#if defined(_MSC_VER)
#include <sstream>

// visual studio is dumb and doesn't let you see the result of std::cout unless you use OutputDebugStringA().
class dbg_stream_for_cout: public std::stringbuf {
    public:
    ~dbg_stream_for_cout();
    int sync();
};
inline dbg_stream_for_cout dbg_printstream;
#endif

void TestPrint();

template<typename ... Args>
void _DebugLogInfo_(const char* file, int lineNumber, Args... args) {
    #if defined(_MSC_VER)
    std::cout.rdbuf(&dbg_printstream);
    #endif

    #ifdef IS_MODULE
    std::cout << "\x1B[33mINFO: (IN MODULE; from " << file << ":" << lineNumber << ")\x1B[37m ";
    #else
    std::cout << "\x1B[33mINFO: (from " << file << ":" << lineNumber << ")\x1B[37m ";
    #endif
    ((std::cout << std::forward<Args>(args)), ...);
    std::cout << "\n";

    std::cout.flush();
}

template<typename ... Args>
void _DebugLogError_(const char* file, int lineNumber, Args... args) {
    #if defined(_MSC_VER)
    std::cout.rdbuf(&dbg_printstream);
    #endif

    #ifdef IS_MODULE
    std::cout << "\x1B[31mERROR: (IN MODULE; from " << file << ":" << lineNumber << ")\x1B[37m ";
    #else
    std::cout << "\x1B[31mERROR: (from " << file << ":" << lineNumber << ")\x1B[37m ";
    #endif
    ((std::cout << std::forward<Args>(args)), ...);
    std::cout << "\n";

    std::cout.flush();
}

#define DebugLogError(...) {_DebugLogError_(__FILE__, __LINE__, __VA_ARGS__);}
#define DebugLogInfo(...) {_DebugLogInfo_(__FILE__, __LINE__, __VA_ARGS__);}

