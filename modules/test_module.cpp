// #define IS_MODULE 1

// #include "../src/gameobjects/component_registry.hpp"

// #include <vector>

// #include "graphics/engine.hpp"
// #include "graphics/shader_program.hpp"
// #include "graphics/material.hpp"
// #include "gameobjects/component_registry.hpp"
// #include "physics/raycast.hpp"
// // #include "../src/physics/engine.hpp"
// #include "gameobjects/rigidbody_component.hpp"
// // #include "../src/lua/lua_handler.hpp"
// #include "gameobjects/lifetime.hpp"
// // #include "../src/network/client.hpp"
// #include "conglomerates/gui.hpp"

// // #include "../src/audio/engine.hpp"
// // #include "../src/audio/sound.hpp"

#include "modules/module_globals_pointers.hpp"
#include "modules/engine_export.hpp"

// #include "debug/log.hpp"
// #include "gameobjects/component_registry.hpp"
// #include "graphics/engine.hpp"
// #include "GLM/gtx/string_cast.hpp"
// #include "graphics/engine.hpp"
// #include "physics/engine.hpp"
// #include "audio/engine.hpp"

#include <iostream>

ModuleTestInterface* test;

extern "C" {

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)

__declspec (dllexport) void LoadGlobals(ModulesGlobalsPointers globals) {
    // ComponentRegistry::SetModuleComponentRegistry(globals.CR);
    // GraphicsEngine::SetModuleGraphicsEngine(globals.GE);
    // MeshGlobals::SetModuleMeshGlobals(globals.MG);
    // SpatialAccelerationStructure::SetModuleSpatialAccelerationStructure(globals.SAS);
    // GuiGlobals::SetModuleGuiGlobals(globals.GG);
    // PhysicsEngine::SetModulePhysicsEngine(globals.PE);
    // AudioEngine::SetModuleAudioEngine(globals.AE);
    test = globals.TEST;
}

__declspec (dllexport) void OnInit() {
    
    std::cout << "OH YEAH LETS GO\n";
    test->PrintTest();
    // auto & GE = GraphicsEngine::Get();
    // auto & CR = ComponentRegistry::Get();
    // auto & PE = PhysicsEngine::Get();
    // auto & AE = AudioEngine::Get();
    // auto & LUA = LuaHandler::Get();

        //GE.camera.position.y = 3;

}

__declspec (dllexport) void OnPostPhysics() {
    

    // auto & GE = GraphicsEngine::Get();

    // printf("Doing a little raycasting.\n");
    
}

__declspec (dllexport) void OnClose() {
    // delete ui;
}

#endif

}
