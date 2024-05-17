#pragma once
#include <iostream>

// TODO: line numbers

template<typename ... Args>
void _DebugLogInfo_(const char* file, int lineNumber, Args... args) {
    std::cout << "\x1B[33mINFO: (from " << file << ":" << lineNumber << ")\x1B[37m ";
    ((std::cout << std::forward<Args>(args)), ...);
    std::cout << "\n";
}

template<typename ... Args>
void _DebugLogError_(const char* file, int lineNumber, Args... args) {
    std::cout << "\x1B[31mERROR: (from " << file << ":" << lineNumber << ")\x1B[37m ";
    ((std::cout << std::forward<Args>(args)), ...);
    std::cout << "\n";
}

#define DebugLogError(Args...) {_DebugLogError_(__FILE__, __LINE__, Args);}
#define DebugLogInfo(Args...) {_DebugLogInfo_(__FILE__, __LINE__, Args);}

