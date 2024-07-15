#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "GLM/gtx/string_cast.hpp"

#include "graphics/gengine.hpp"
#include "graphics/mesh.hpp"
#include "graphics/shader_program.hpp"
#include "graphics/material.hpp"
#include "gameobjects/component_registry.hpp"
#include "physics/pengine.hpp"
#include "gameobjects/rigidbody_component.hpp"
#include "lua/lua_handler.hpp"
#include "gameobjects/lifetime.hpp"
#include "conglomerates/gui.hpp"

#include "audio/aengine.hpp"
#include "audio/sound.hpp"

#include "debug/log.hpp"
#include "physics/spatial_acceleration_structure.hpp"

#include "modules/module.hpp"

#include "non-engine/game.hpp"

//#include "FastNoise/FastNoise.h"

using namespace std;

double timeAtWhichExitProcessStarted = 0;


void AtExit() {        
    LogElapsed(timeAtWhichExitProcessStarted, "Exit process elapsed ");
    DebugLogInfo("Program ran successfully. Exiting.");
}

// just prints about unhandled exceptions because visual studio won't.
void TerminateHandler() {
    DebugLogError("Fatal error: Unhandled exception.");
    try {
        std::rethrow_exception(std::current_exception());
    }
    catch (const std::exception& e) {
        DebugLogError("Exception: ", e.what());
    }
    catch (...) {
        DebugLogError("Unknown exception object.");
    }
    abort();
}

int main(int numArgs, const char *argPtrs[]) {
    
    DebugLogInfo("Main function reached.");

    std::set_terminate(TerminateHandler);
    atexit(AtExit);

    // TODO: order does matter here, formalize that (TODO: wait it does? why?)
    auto & GE = GraphicsEngine::Get();
    auto & PE = PhysicsEngine::Get();
    auto & AE = AudioEngine::Get();
    auto & LUA = LuaHandler::Get();
    auto & CR = ComponentRegistry::Get();

    atexit(Module::CloseAll); // this is placed here, after the component registry is initialized, because that guarantees that modules' references to gameobjects are destroyed before the gameobjects are (because static destructors/at exits are called in reverse order)

    DebugLogInfo("Calling gameInit().");
    GameInit();

    DebugLogInfo("Starting main loop.");

    // The mainloop uses a fixed physics timestep, but renders as much as possible
    // TODO: right now rendering extrapolates positions for rigidbodies, we could possibly do interpolation??? would require storing old positions tho so idk
    // TODO: options for other mainloops
    // TODO: max framerate option in leiu of vsync
    const double SIMULATION_TIMESTEP = 1.0/60.0; // number of seconds simulation is stepped by every frame

    const unsigned int N_PHYSICS_ITERATIONS = 1; // bigger number = higher quality physics simulation, although do we ever want this? what about just decrease sim timestep?
    double previousTime = Time();
    double physicsLag = 0.0; // how many seconds behind the simulation is. Before rendering, we check if lag is > SIMULATION_TIMESTEP in ms and if so, simulate stuff.

    // TODO: find a way to move pausing out of main.cpp
    bool physicsPaused = true;
    bool wf = false;

    GE.window.InputDown.Connect([&physicsPaused, &wf, &GE](InputObject input) {
        if (input.input == InputObject::Space) {
            physicsPaused = !physicsPaused;
        }
        else if (input.input == InputObject::Escape) {
            GE.window.Close();
        }
        else if (input.input == InputObject::Tab) {
            GE.window.SetMouseLocked(!GE.window.IsMouseLocked());
        }
        else if (input.input == InputObject::Alt) {
            wf = !wf;
            GE.SetWireframeEnabled(wf);
        }
    });

    while (!GE.ShouldClose()) 
    {
       
        double currentTime = Time();
        double elapsedTime = currentTime - previousTime;
        previousTime = currentTime; 
        physicsLag += elapsedTime; // time has passed and thus the simulation is behind
        
        // printf("Processing events.\n");
        GE.window.Update(); // window/input event processing

        BaseEvent::FlushEventQueue();
        LUA.OnFrameBegin();
        LUA.PrePhysicsCallbacks();
        Module::PrePhysics();

        if (physicsLag >= SIMULATION_TIMESTEP) { // make sure stepping simulation won't put it ahead of realtime
            // printf("Updating SAS.\n");

            SpatialAccelerationStructure::Get().Update();

            if (!physicsPaused) {
                // physicsPaused = true;
                // printf("Stepping PE.\n");
                
                PE.prePhysicsEvent.Fire(SIMULATION_TIMESTEP);
                BaseEvent::FlushEventQueue(); // if we don't do this, if firing the event is meant to (for example) change velocities, it won't apply until next frame.

                for (unsigned int i = 0; i < N_PHYSICS_ITERATIONS; i++) {
                    PE.Step(SIMULATION_TIMESTEP/2.0/N_PHYSICS_ITERATIONS);
                    PE.Step(SIMULATION_TIMESTEP/4.0/N_PHYSICS_ITERATIONS);
                    PE.Step(SIMULATION_TIMESTEP/8.0/N_PHYSICS_ITERATIONS);
                    PE.Step(SIMULATION_TIMESTEP/8.0/N_PHYSICS_ITERATIONS);
                    // PE.Step(SIMULATION_TIMESTEP);
                }

                PE.postPhysicsEvent.Fire(SIMULATION_TIMESTEP);
                BaseEvent::FlushEventQueue();
            }
            
            physicsLag -= SIMULATION_TIMESTEP;
        }

        BaseEvent::FlushEventQueue();
        LUA.PostPhysicsCallbacks();
        Module::PostPhysics();
        
        AE.Update();

        BaseEvent::FlushEventQueue();
        LUA.PreRenderCallbacks();
        Gui::UpdateBillboardGuis();

        GE.RenderScene(elapsedTime);

        UpdateLifetimes();

        // TODO: unsure about placement of flip buffers? 
        // i think this yields until GPU done drawing and image on screen
        // could/should we do something to try and do physics or something while GPU working? or are we already? 
        // printf("Flipping buffers.\n");
        GE.window.FlipBuffers();

        BaseEvent::FlushEventQueue();
        LUA.PostRenderCallbacks();
        Module::PostRender();

        // LogElapsed(currentTime, "Frame elapsed");

    }

    timeAtWhichExitProcessStarted = Time();
    DebugLogInfo("Beginning exit process.");

    // delete ui;
    
    return EXIT_SUCCESS;
}