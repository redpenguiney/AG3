#pragma once

#include <cstdio>
#define LOG_LINE_REACHED
#define DEBUG_ASSERT_ENABLED
#include <iostream>
#include <chrono>
inline void DebugLogLineReached(const char *file, int line){
    #ifdef LOG_LINE_REACHED
    std::cout << "\n Line " << line << " in file " << file << " was reached successfully.";
    #endif
}
#define DebugLogLineReached() DebugLogLineReached(__FILE__, __LINE__)

// void _assert(const char *file, int line, bool condition, std::string failureMessage) {
//     #ifdef DEBUG_ASSERT_ENABLED
//     if (!condition) {
//         std::cout << "\nAssertion at line " << line << " in " << file << " failed: " << failureMessage; 
//     }
    
//     #endif
// }
// #define assert() _assert(__FILE__, __LINE__, )

// returns time in milliseconds
inline double Time() {
    using namespace std::chrono;
    duration<double, std::milli> time = high_resolution_clock::now().time_since_epoch();
    return time.count();
}

// prints time between start and now
inline void LogElapsed(double start, std::string message = "\nElapsed ") {
    std::cout << message << (Time() - start) << "ms.";
}