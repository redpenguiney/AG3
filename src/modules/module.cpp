#include "module.hpp"
#include "../debug/log.hpp"

#if defined(__WIN64) || defined(__WIN32)
#include "windows.h"
#else
#error unsupported module platform
#endif

#if defined (__WIN64) || defined(__WIN32)
// returns nullptr if no function
void* LoadFunc(const HMODULE& windowsModule, const char* funcName) {
    FARPROC functionAddress = GetProcAddress(windowsModule, funcName);
    return (void*)functionAddress;
}
#else
#error unsupported module platform
#endif

void Module::PrePhysics() {
    for (auto & module: LOADED_MODULES) {
        if (module.onPrePhysics.has_value()) {
            (*(module.onPrePhysics))();
        }
    }
}

void Module::PostPhysics() {
    for (auto & module: LOADED_MODULES) {
        if (module.onPostPhysics.has_value()) {
            (*(module.onPostPhysics))();
        }
    }
}

void Module::PostRender() {
    for (auto & module: LOADED_MODULES) {
        if (module.onPostRender.has_value()) {
            (*(module.onPostRender))();
        }
    }
}

Module::Module(const char* filepath) {
    #if defined(__WIN64) || defined(__WIN32)
    module = new HMODULE;
    *(HMODULE*)module = LoadLibrary(filepath);
    if (!module) {
        DebugLogError("Failed to load module, ", filepath);
    }
    auto init = LoadFunc(*(HMODULE*)module, "OnInit");
    if (init) {
        onInit = (void(*)())init;
        (*onInit)();
    }
    auto postRender = LoadFunc(*(HMODULE*)module, "OnPostRender");
    if (postRender) {
        onPostRender = (void(*)())postRender;
    }
    auto prePhys = LoadFunc(*(HMODULE*)module, "OnPrePhysics");
    if (prePhys) {
        onPrePhysics = (void(*)())prePhys;
    }
    auto postPhys = LoadFunc(*(HMODULE*)module, "OnPostPhysics");
    if (postPhys) {
        onPostPhysics = (void(*)())postPhys;
    }
    auto close = LoadFunc(*(HMODULE*)module, "OnClose");
    if (close) {
        onClose = (void(*)())close;
    }
    #else
    #error unsupported module platform
    #endif

    
}

Module::~Module() {
    (*onClose)();
    FreeLibrary(*(HMODULE*)module);
    delete (HMODULE*)module;
}