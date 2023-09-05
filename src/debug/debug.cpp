#pragma once
#define LOG_LINE_REACHED
#include <iostream>
#include <chrono>
void DebugLogLineReached(const char *file, int line){
    #ifdef LOG_LINE_REACHED
    std::cout << "\n Line " << line << " in file " << file << " was reached successfully.";
    #endif
}
#define DebugLogLineReached() DebugLogLineReached(__FILE__, __LINE__)

// returns time in milliseconds
double Time() {
    using namespace std::chrono;
    duration<double, std::milli> time = high_resolution_clock::now().time_since_epoch();
    return time.count();
}