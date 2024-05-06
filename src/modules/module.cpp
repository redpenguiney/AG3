#include "module.hpp"
#include "../debug/log.hpp"
#include "coroutine.hpp"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include "windows.h"
#else
#error unsupported module platform
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
// returns nullptr if no function
void* LoadFunc(const HMODULE& windowsModule, const char* funcName) {
    FARPROC functionAddress = GetProcAddress(windowsModule, funcName);
    // DebugLogInfo("Func name ", funcName, " has proc address ", functionAddress);
    return (void*)functionAddress;
}
#else
#error unsupported module platform
#endif

void Module::LoadModule(const char *filepath) {
    auto module = Module(filepath);
    LOADED_MODULES.push_back(std::move(module));
}

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

std::list<Coroutine> runningCoroutines;

void RunHook(void(*func)()) {
    Coroutine f = [func]() -> Coroutine {
        func();
        co_return;
    }();
    f.resume();
}



Module::Module(const char* filepath) {
    #if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    module = new HMODULE;
    *(HMODULE*)module = LoadLibrary(filepath);
    if ((*(HMODULE*)module) == nullptr) {
        LPVOID lpMsgBuf;
        DWORD dw = GetLastError(); 

        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &lpMsgBuf,
            0, NULL );

        DebugLogError("Failed to load module, ", filepath, "; ", (char*)lpMsgBuf);
    }
    auto init = LoadFunc(*(HMODULE*)module, "OnInit");
    if (init) {
        onInit = (void(*)())init;
        
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

    if (onInit.has_value()) {
        
        RunHook(*onInit);

        DebugLogInfo("co exited");
    }
}

void Module::CloseAll() {
    LOADED_MODULES.clear();
}

Module::~Module() {
    (*onClose)();
    #if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    FreeLibrary(*(HMODULE*)module);
    delete (HMODULE*)module;
    #else
    #error unsupported module platform
    #endif
}