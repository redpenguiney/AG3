#include <cmath>
#include<cstdlib>
#include<cstdio>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "graphics/engine.hpp"
#include "graphics/shader_program.hpp"
#include "graphics/material.hpp"
#include "gameobjects/component_registry.hpp"
#include "physics/raycast.hpp"
#include "physics/engine.hpp"
#include "gameobjects/rigidbody_component.hpp"
#include "lua/lua_handler.hpp"
#include "gameobjects/lifetime.hpp"

using namespace std;

int main(int numArgs, char *argPtrs[]) {
    std::printf("Main function reached.\n");

    // shouldn't actually matter if these lines exist, and if it does fix that please
    auto & GE = GraphicsEngine::Get();
    auto & PE = PhysicsEngine::Get();
    auto & LUA = LuaHandler::Get();
    //GE.camera.position.y = 3;

    auto m = Mesh::FromFile("../models/rainbowcube.obj", true, true, -1.0, 1.0, 16384);
    auto [grassTextureZ, grassMaterial] = Material::New({TextureCreateParams {.texturePaths = {"../textures/grass.png"}, .format = RGB, .usage = ColorMap}, TextureCreateParams {.texturePaths = {"../textures/crate_specular.png"}, .format = Grayscale, .usage = SpecularMap}}, TEXTURE_2D);

    //std::printf("ITS %u %u\n", m->meshId, grassMaterial->id);
    auto [brickTextureZ, brickMaterial] = Material::New({
        TextureCreateParams {.texturePaths = {"../textures/ambientcg_bricks085/color.jpg"}, .format = RGBA, .usage = ColorMap}, 
        TextureCreateParams {.texturePaths = {"../textures/ambientcg_bricks085/roughness.jpg"}, .format = Grayscale, .usage = SpecularMap}, 
        TextureCreateParams {.texturePaths = {"../textures/ambientcg_bricks085/normal_gl.jpg"}, .format = RGB, .usage = NormalMap}, 
        // TextureCreateParams {.texturePaths = {"../textures/ambientcg_bricks085/displacement.jpg"}, .format = Grayscale, .usage = DisplacementMap}
        }, TEXTURE_2D);

    {
        GameobjectCreateParams params({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::RenderComponentBitIndex, ComponentRegistry::ColliderComponentBitIndex});
        params.meshId = m->meshId;
        params.materialId = grassMaterial->id;
        auto floor = ComponentRegistry::NewGameObject(params);
        floor->transformComponent->SetPos({0, 0, 0});
        floor->transformComponent->SetRot(glm::vec3 {0, 0, 0.0});
        floor->colliderComponent->elasticity = 1.0;
        floor->transformComponent->SetScl({9, 1, 9});
        floor->renderComponent->SetColor({0, 1, 0, 0.5});
        floor->renderComponent->SetTextureZ(grassTextureZ);
    }

    // {
    //     GameobjectCreateParams params({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::RenderComponentBitIndex, ComponentRegistry::ColliderComponentBitIndex});
    //     params.meshId = m->meshId;
    //     params.materialId = brickMaterial->id;
    //     // auto wall1 = ComponentRegistry::NewGameObject(params);
    //     // wall1->transformComponent->SetPos({4, 4, 0});
    //     // wall1->transformComponent->SetRot(glm::vec3 {0, 0, 0.0});
    //     // wall1->colliderComponent->elasticity = 1.0;
    //     // wall1->transformComponent->SetScl({1, 8, 8});
    //     // wall1->renderComponent->SetColor({1, 1, 1, 1});
    //     // wall1->renderComponent->SetTextureZ(brickTextureZ);
    //     auto wall2 = ComponentRegistry::NewGameObject(params);
    //     wall2->transformComponent->SetPos({-4, 4, 0});
    //     wall2->transformComponent->SetRot(glm::vec3 {0, 0, 0.0});
    //     wall2->colliderComponent->elasticity = 1.0;
    //     wall2->transformComponent->SetScl({1, 8, 8});
    //     wall2->renderComponent->SetColor({1, 1, 1, 0.5});
    //     wall2->renderComponent->SetTextureZ(brickTextureZ);
    // }

    auto skyboxFaces = vector<std::string>( // TODO: need to make clear what order skybox faces go in
    { 
        "../textures/sky/bottom.png",
        "../textures/sky/bottom.png",
        "../textures/sky/bottom.png",
        "../textures/sky/top.png",
        "../textures/sky/bottom.png",
        "../textures/sky/bottom.png"
    });

    {
        // auto [index, sky_m_ptr] = Material::New({TextureCreateParams {.texturePaths = skyboxFaces, .format = RGBA, .usage = ColorMap}}, TEXTURE_CUBEMAP);
        // GE.skyboxMaterial = sky_m_ptr;
        // GE.skyboxMaterialLayer = index;
    }
    GE.debugFreecamPos = glm::vec3(0, 3, 8);
    
    
    int i = 0;
    for (int x = 0; x < 1; x++) {
        for (int y = 0; y < 1; y++) {
            for (int z = 0; z < 1; z++) {
                GameobjectCreateParams params({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::RenderComponentBitIndex, ComponentRegistry::ColliderComponentBitIndex, ComponentRegistry::RigidbodyComponentBitIndex});
                params.meshId = m->meshId;
                params.materialId = brickMaterial->id;
                auto g = ComponentRegistry::NewGameObject(params);
                g->transformComponent->SetPos({x * 3, 5 + y * 3, z * 3});
                g->colliderComponent->elasticity = 1.0;
                g->transformComponent->SetRot(glm::quat(glm::vec3(1.0, 0.0, 0.0)));
                g->rigidbodyComponent->angularVelocity = {1.0, 0.0, 0.0};
                //g->transformComponent->SetScl(glm::dvec3(2, 2, 2));
                g->renderComponent->SetColor(glm::vec4(1, 1, 1, 1));
                g->renderComponent->SetTextureZ(brickTextureZ);
                g->name = std::string("Gameobject #") + to_string(i);
                i++;
            } 
        }
    }

    
    // make light
    {GameobjectCreateParams params({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::PointlightComponentBitIndex, ComponentRegistry::RenderComponentBitIndex, ComponentRegistry::ColliderComponentBitIndex});
    params.meshId = m->meshId;
    params.materialId = 0;
    auto coolLight = ComponentRegistry::NewGameObject(params);
    coolLight->renderComponent->SetTextureZ(-1);
    coolLight->transformComponent->SetPos({0, 10, 10});
    coolLight->pointLightComponent->SetRange(200);
    coolLight->pointLightComponent->SetColor({1, 1, 1});}
    {GameobjectCreateParams params({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::PointlightComponentBitIndex, ComponentRegistry::RenderComponentBitIndex, ComponentRegistry::ColliderComponentBitIndex});
    params.meshId = m->meshId;
    params.materialId = grassMaterial->id;
    auto coolLight = ComponentRegistry::NewGameObject(params);
    coolLight->renderComponent->SetTextureZ(-1);
    coolLight->transformComponent->SetPos({30, 10, 30});
    coolLight->pointLightComponent->SetRange(100);
    coolLight->pointLightComponent->SetColor({1, 1, 1});}
    
    
    GE.debugFreecamEnabled = true;
    GE.window.SetMouseLocked(true);
    
    glPointSize(4.0); // debug thing, ignore
    glLineWidth(2.0);

    printf("Starting main loop.\n");

    // The mainloop uses a fixed physics timestep, but renders as much as possible
    // TODO: right now rendering extrapolates positions for rigidbodies, we could possibly do interpolation??? would require storing old positions tho so idk
    // TODO: options for other mainloops
    // TODO: max framerate option in leiu of vsync
    const double SIMULATION_TIMESTEP = 1.0/60.0; // number of seconds simulation is stepped by every frame

    const unsigned int N_PHYSICS_ITERATIONS = 1; // bigger number = higher quality physics simulation, although do we ever want this? what about just increase sim timestep?
    double previousTime = Time();
    double physicsLag = 0.0; // how many seconds behind the simulation is. Before rendering, we check if lag is > SIMULATION_TIMESTEP in ms and if so, simulate stuff.

    bool physicsPaused = false;

    while (!GE.ShouldClose()) 
    {
        // std::printf("ok %f %f \n", GE.debugFreecamPitch, GE.debugFreecamYaw);
        double currentTime = Time();
        double elapsedTime = currentTime - previousTime;
        previousTime = currentTime; 
        physicsLag += elapsedTime; // time has passed and thus the simulation is behind
        
        // printf("Processing events.\n");
        GE.window.Update(); // event processing

        if (physicsLag >= SIMULATION_TIMESTEP) { // make sure stepping simulation won't put it ahead of realtime
            // printf("Updating SAS.\n");

            SpatialAccelerationStructure::Get().Update();

            if (!physicsPaused) {
                // physicsPaused = true;
                // printf("Stepping PE.\n");
                for (unsigned int i = 0; i < N_PHYSICS_ITERATIONS; i++) {
                    PE.Step(SIMULATION_TIMESTEP/N_PHYSICS_ITERATIONS);
                }
            }
            
            physicsLag -= SIMULATION_TIMESTEP;
        }
        

        //printf("Doing a little raycasting.\n");
        if (LMB_DOWN) {
            auto castResult = Raycast(GE.debugFreecamPos, LookVector(glm::radians(GE.debugFreecamPitch), glm::radians(GE.debugFreecamYaw)));
            if (castResult.hitObject != nullptr && castResult.hitObject->rigidbodyComponent) {
                //std::cout << "Hit object " << castResult.hitObject->name << " \n";
                castResult.hitObject->rigidbodyComponent->velocity += castResult.hitNormal * 0.4;
                //castResult.hitObject->transformComponent->SetPos(castResult.hitObject->transformComponent->position() + castResult.hitNormal * 0.02);
            }
            else {
                //std::cout << "LMB_DOWN but not hitting anything.\n";
            }

        }

        if (PRESS_BEGAN_KEYS[GLFW_KEY_SPACE]) {
            physicsPaused = !physicsPaused;
        }
        if (PRESS_BEGAN_KEYS[GLFW_KEY_ESCAPE]) {
            GE.window.Close();
        }
        if (PRESS_BEGAN_KEYS[GLFW_KEY_TAB]) {
            GE.window.SetMouseLocked(!GE.window.mouseLocked);
        }
        
        // printf("Rendering scene.\n");
        GE.RenderScene();


        UpdateLifetimes();

        // TODO: unsure about placement of flip buffers? 
        // i think this yields until GPU done drawing and image on screen
        // could/should we do something to try and do physics or something while GPU Working? or are we already? 
        // printf("Flipping buffers.\n");
        GE.window.FlipBuffers();

        // if (g->transformComponent->position().y <= 1.0) {
        //     while (true) {}
        // }

        // std::printf("ok %f %f \n", GE.debugFreecamPitch, GE.debugFreecamYaw);
        // GE.window.Close();

        // LogElapsed(currentTime, "Frame elapsed");

    }
    printf("Beginning exit process.\n");

    auto start = Time();

    // It's very important that this function runs before GE, SAS, etc. are destroyed, and it's very important that other shared_ptrs to gameobjects outside of the GAMEOBJECTS map are gone (meaning lua needs to be deleted before this code runs)
    ComponentRegistry::CleanupComponents(); printf("Cleaned up all gameobjects.\n");

    LogElapsed(start, "Exit process elapsed ");
    printf("Program ran successfully (unless someone's destructor crashes after this print statement). Exiting.\n");
    return EXIT_SUCCESS;
}