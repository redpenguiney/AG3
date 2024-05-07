#include "module.hpp"
#include "../debug/log.hpp"

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
    module.internalModule = nullptr; // when module gets copied, this local variable will call its destructor and delete internal module against our will if we don't set this
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


void RunHook(void(*func)()) {
    func();
}



Module::Module(const char* filepath) {
    internalModule = nullptr;
    #if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    internalModule = new HMODULE;
    *(HMODULE*)internalModule = LoadLibrary(filepath);
    if ((*(HMODULE*)internalModule) == nullptr) {
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
        internalModule = nullptr;
    }
    else {
        auto init = LoadFunc(*(HMODULE*)internalModule, "OnInit");
        if (init) {
            onInit = (void(*)())init;
            
        }
        auto postRender = LoadFunc(*(HMODULE*)internalModule, "OnPostRender");
        if (postRender) {
            onPostRender = (void(*)())postRender;
        }
        auto prePhys = LoadFunc(*(HMODULE*)internalModule, "OnPrePhysics");
        if (prePhys) {
            onPrePhysics = (void(*)())prePhys;
        }
        auto postPhys = LoadFunc(*(HMODULE*)internalModule, "OnPostPhysics");
        if (postPhys) {
            onPostPhysics = (void(*)())postPhys;
        }
        auto close = LoadFunc(*(HMODULE*)internalModule, "OnClose");
        if (close) {
            onClose = (void(*)())close;
        }
    }
    
    #else
    #error unsupported module platform
    #endif

    if (onInit.has_value()) {
        
        RunHook(*onInit);

    }
}

void Module::CloseAll() {
    LOADED_MODULES.clear();
}

Module::~Module() {
    
    if (internalModule != nullptr) {
        (*onClose)();

        #if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    
        FreeLibrary(*(HMODULE*)internalModule);
        delete (HMODULE*)internalModule;

        #else
        #error unsupported module platform
        #endif
    }
    
    
}