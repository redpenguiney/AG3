#pragma once
#include <iostream>

// TODO: line numbers

template<typename ... Args>
void DebugLogInfo(Args... args) {
    std::cout << "\x1B[33mINFO: \x1B[37m";
    ((std::cout << std::forward<Args>(args)), ...);
    std::cout << "\n";
}

template<typename ... Args>
void DebugLogError(Args... args) {
    std::cout << "\x1B[31mERROR: \x1B[37m";
    ((std::cout << std::forward<Args>(args)), ...);
    std::cout << "\n";
}

