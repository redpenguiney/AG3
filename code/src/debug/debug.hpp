#pragma once

#include <cstdio>
#define LOG_LINE_REACHED
#define DEBUG_ASSERT_ENABLED
#include <iostream>
#include "../utility/utility.hpp"
#include "log.hpp"
inline void DebugLogLineReached(const char *file, int line){
    #ifdef LOG_LINE_REACHED
    std::cout << "\nLine " << line << " in file " << file << " was reached successfully.";
    #endif
}
#define DebugLogLineReached() DebugLogLineReached(__FILE__, __LINE__)

// void _Assert(const char *file, int line, bool condition, std::string failureMessage) {
//     #ifdef DEBUG_ASSERT_ENABLED
//     if (!condition) {
//         std::cout << "\nAssertion at line " << line << " in " << file << " failed: " << failureMessage; 
//     }
    
//     #endif
// }
// #define Assert() _Assert(__FILE__, __LINE__, )

// prints time between start and now
inline void LogElapsed(double start, std::string message = "\nElapsed ") {
    DebugLogInfo(message, (Time() - start), "ms.");
}