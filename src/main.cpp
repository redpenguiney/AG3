#include <cmath>
#include<cstdlib>
#include<cstdio>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "../external_headers/GLM/gtx/string_cast.hpp"

#include "graphics/engine.hpp"
#include "graphics/shader_program.hpp"
#include "graphics/material.hpp"
#include "gameobjects/component_registry.hpp"
#include "physics/raycast.hpp"
#include "physics/engine.hpp"
#include "gameobjects/rigidbody_component.hpp"
#include "lua/lua_handler.hpp"
#include "gameobjects/lifetime.hpp"
#include "network/client.hpp"
#include "conglomerates/gui.hpp"

#include "audio/engine.hpp"
#include "audio/sound.hpp"

#include "debug/log.hpp"
#include "src/gameobjects/component_registry.hpp"
#include "src/graphics/engine.hpp"

#include "modules/module.hpp"

using namespace std;

double timeAtWhichExitProcessStarted = 0;

void AtExit() {
    DebugLogInfo("Closing modules.");
    Module::CloseAll();
    // DebugLogInfo("Cleaning up all gameobjects.");
    
    LogElapsed(timeAtWhichExitProcessStarted, "Exit process elapsed ");
    DebugLogInfo("Program ran successfully. Exiting.");
}

int main(int numArgs, const char *argPtrs[]) {
    

    DebugLogInfo("Main function reached.");

    atexit(AtExit); DebugLogInfo("0");

    // TODO: shouldn't actually matter if these lines exist, and if it does fix that please
    auto & GE = GraphicsEngine::Get(); DebugLogInfo("1");
    auto & PE = PhysicsEngine::Get(); DebugLogInfo("2");
    auto & AE = AudioEngine::Get(); DebugLogInfo("3");
    auto & LUA = LuaHandler::Get(); DebugLogInfo("4");
    auto & CR = ComponentRegistry::Get(); DebugLogInfo("5");

    Module::LoadModule("..\\modules\\libtest_module.dll");



    DebugLogInfo("Starting main loop.");

    // The mainloop uses a fixed physics timestep, but renders as much as possible
    // TODO: right now rendering extrapolates positions for rigidbodies, we could possibly do interpolation??? would require storing old positions tho so idk
    // TODO: options for other mainloops
    // TODO: max framerate option in leiu of vsync
    const double SIMULATION_TIMESTEP = 1.0/60.0; // number of seconds simulation is stepped by every frame

    const unsigned int N_PHYSICS_ITERATIONS = 1; // bigger number = higher quality physics simulation, although do we ever want this? what about just decrease sim timestep?
    double previousTime = Time();
    double physicsLag = 0.0; // how many seconds behind the simulation is. Before rendering, we check if lag is > SIMULATION_TIMESTEP in ms and if so, simulate stuff.

    bool physicsPaused = true;

    

    while (!GE.ShouldClose()) 
    {
        // wall1->transformComponent->SetRot(wall1->transformComponent->Rotation() * glm::quat(glm::vec3(0.0, glm::radians(1.0), 0.0)));
        // // wall1->transformComponent->SetScl(wall1->transformComponent->Scale() + glm::vec3(0.05, 0.05, 0.05));
        // wall1->transformComponent->SetPos(wall1->transformComponent->Position() + glm::dvec3 {0.1, 0.0, 0.0});

        // std::printf("ok %f %f \n", GE.debugFreecamPitch, GE.debugFreecamYaw);
        double currentTime = Time();
        double elapsedTime = currentTime - previousTime;
        previousTime = currentTime; 
        physicsLag += elapsedTime; // time has passed and thus the simulation is behind
        
        // printf("Processing events.\n");
        GE.window.Update(); // event processing

        LUA.OnFrameBegin();
        LUA.PrePhysicsCallbacks();
        Module::PrePhysics();

        if (physicsLag >= SIMULATION_TIMESTEP) { // make sure stepping simulation won't put it ahead of realtime
            // printf("Updating SAS.\n");

            SpatialAccelerationStructure::Get().Update();

            if (!physicsPaused) {
                // physicsPaused = true;
                // printf("Stepping PE.\n");
                for (unsigned int i = 0; i < N_PHYSICS_ITERATIONS; i++) {
                    PE.Step(SIMULATION_TIMESTEP/2.0);
                    PE.Step(SIMULATION_TIMESTEP/4.0);
                    PE.Step(SIMULATION_TIMESTEP/8.0);
                    PE.Step(SIMULATION_TIMESTEP/8.0);
                    // PE.Step(SIMULATION_TIMESTEP);
                }

                
            }
            
            physicsLag -= SIMULATION_TIMESTEP;
        }

        LUA.PostPhysicsCallbacks();
        Module::PostPhysics();
        
        

        // printf("Doing a little raycasting.\n");
        if (GE.window.LMB_DOWN) {
            auto castResult = Raycast(GE.debugFreecamCamera.position, LookVector(glm::radians(GE.debugFreecamPitch), glm::radians(GE.debugFreecamYaw)));
            
            if (castResult.hitObject != nullptr) {
                // std::cout << "Hit object " << castResult.hitObject->name << ", normal is " << glm::to_string(castResult.hitNormal) << " \n";
            }

            if (castResult.hitObject != nullptr && castResult.hitObject->rigidbodyComponent) {
                
                castResult.hitObject->rigidbodyComponent->velocity += castResult.hitNormal * 0.4;
                //castResult.hitObject->transformComponent->SetPos(castResult.hitObject->transformComponent->position + castResult.hitNormal * 0.02);
            }
            else {
                // std::cout << "LMB_DOWN but not hitting anything.\n";
            }

        }


        if (GE.window.PRESS_BEGAN_KEYS[GLFW_KEY_SPACE]) {
            physicsPaused = !physicsPaused;
        }
        if (GE.window.PRESS_BEGAN_KEYS[GLFW_KEY_ESCAPE]) {
            GE.window.Close();
        }
        if (GE.window.PRESS_BEGAN_KEYS[GLFW_KEY_TAB]) {
            GE.window.SetMouseLocked(!GE.window.mouseLocked);
        }
        
        AE.Update();

        // printf("Rendering scene.\n");
        LUA.PreRenderCallbacks();
        Gui::UpdateBillboardGuis();
        GE.RenderScene();

        UpdateLifetimes();

        // TODO: unsure about placement of flip buffers? 
        // i think this yields until GPU done drawing and image on screen
        // could/should we do something to try and do physics or something while GPU Working? or are we already? 
        // printf("Flipping buffers.\n");
        GE.window.FlipBuffers();

        LUA.PostRenderCallbacks();
        Module::PostRender();

        // if (g->transformComponent->position.y <= 1.0) {
        //     while (true) {}
        // }

        // std::printf("ok %f %f \n", GE.debugFreecamPitch, GE.debugFreecamYaw);
        // GE.window.Close();

        // LogElapsed(currentTime, "Frame elapsed");

    }

    

    timeAtWhichExitProcessStarted = Time();
    DebugLogInfo("Beginning exit process.");

    
    return EXIT_SUCCESS;
}