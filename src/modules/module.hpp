#pragma once
#include <memory>
#include <optional>
#include <string>
#include <list>

class GraphicsEngine;
class ComponentRegistry;
class PhysicsEngine;
class SpatialAccelerationStructure;
class AudioEngine;

extern "C" {
    struct ModulesGlobalsPointers {
        GraphicsEngine* GE;
        ComponentRegistry* CR;
        PhysicsEngine* PE;
        SpatialAccelerationStructure* SAS;
        AudioEngine* AE;
    };
}

class Module {
    public:

    static void PrePhysics();
    static void PostPhysics();
    static void PostRender();

    // Module.onInit will be immediately called if it exists, so be careful
    static void LoadModule(const char* filepath);
    static void CloseAll();
    
    Module(const Module&) = delete;
    constexpr Module(Module&&) = default;
    ~Module();

    private:

    static inline std::list<Module> LOADED_MODULES;

    Module(const char* filepath);

    std::optional<void(*)()> onInit;
    std::optional<void(*)()> onPrePhysics;
    std::optional<void(*)()> onPostPhysics;
    std::optional<void(*)()> onPostRender;
    std::optional<void(*)()> onClose;

    // void* to avoid inclusion of windows header, pointer to HMODULE (loaded dll) on windows, or to its equivalent on other operating systems
    void* internalModule;
};