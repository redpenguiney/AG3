#include <cmath>
#include<cstdlib>
#include<cstdio>
#include <limits>
#include <memory>
#include <string>
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

using namespace std;

int main(int numArgs, char *argPtrs[]) {
    std::printf("Main function reached.\n");

    // TODO: shouldn't actually matter if these lines exist, and if it does fix that please
    auto & GE = GraphicsEngine::Get();
    auto & PE = PhysicsEngine::Get();
    // auto & LUA = LuaHandler::Get();
    // LUA.RunString("print(\"Hello!\")");
    //GE.camera.position.y = 3;

    auto m = Mesh::FromFile("../models/rainbowcube.obj", MeshVertexFormat::Default(), -1.0, 1.0, 16384);
    auto [grassTextureZ, grassMaterial] = Material::New({TextureCreateParams {{"../textures/grass.png",}, Texture::ColorMap}, TextureCreateParams {{"../textures/crate_specular.png",}, Texture::SpecularMap}}, Texture::Texture2D);

    std::cout << "Created cube mesh vertices:\n ";
    for (auto & v: m->vertices) {
        std::cout << v << ", ";
    }
    std::cout << ".\n";

    //std::printf("ITS %u %u\n", m->meshId, grassMaterial->id);
    auto [brickTextureZ, brickMaterial] = Material::New({
        TextureCreateParams {{"../textures/ambientcg_bricks085/color.jpg",}, Texture::ColorMap}, 
        TextureCreateParams {{"../textures/ambientcg_bricks085/roughness.jpg",}, Texture::SpecularMap}, 
        TextureCreateParams {{"../textures/ambientcg_bricks085/normal_gl.jpg",}, Texture::NormalMap}, 
        // TextureCreateParams {.texturePaths = {"../textures/ambientcg_bricks085/displacement.jpg"}, .format = Grayscale, .usage = DisplacementMap}
        }, Texture::Texture2D);

    {
        GameobjectCreateParams params({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::RenderComponentBitIndex, ComponentRegistry::ColliderComponentBitIndex});
        params.meshId = m->meshId;
        params.materialId = grassMaterial->id;

        auto floor = ComponentRegistry::NewGameObject(params);
        floor->transformComponent->SetPos({0, 0, 0});
        floor->transformComponent->SetRot(glm::vec3 {0.0, 0, glm::radians(5.0)});
        floor->colliderComponent->elasticity = 0.9;
        floor->transformComponent->SetScl({100, 1, 100});
        floor->renderComponent->SetColor({0, 1, 0, 0.5});
        floor->renderComponent->SetTextureZ(grassTextureZ);
        floor->name = "ah yes the floor here is made of floor";
    }

    {
        GameobjectCreateParams params({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::RenderComponentBitIndex, ComponentRegistry::ColliderComponentBitIndex});
        params.meshId = m->meshId;
        params.materialId = brickMaterial->id;
        auto wall1 = ComponentRegistry::NewGameObject(params);
        wall1->transformComponent->SetPos({4, 4, 0});
        wall1->transformComponent->SetRot(glm::vec3 {0, 0, 0.0});
        wall1->colliderComponent->elasticity = 1.0;
        wall1->transformComponent->SetScl({1, 8, 8});
        wall1->renderComponent->SetColor({1, 1, 1, 1});
        wall1->renderComponent->SetTextureZ(brickTextureZ);
        wall1->name = "wall";
        auto wall2 = ComponentRegistry::NewGameObject(params);
        wall2->transformComponent->SetPos({-4, 4, 0});
        wall2->transformComponent->SetRot(glm::vec3 {0, 0, 0.0});
        wall2->colliderComponent->elasticity = 1.0;
        wall2->transformComponent->SetScl({1, 8, 8});
        wall2->renderComponent->SetColor({1, 1, 1, 1.0});
        wall2->renderComponent->SetTextureZ(brickTextureZ);
        wall2->name = "wall";
        auto wall3 = ComponentRegistry::NewGameObject(params);
        wall3->transformComponent->SetPos({0, 4, 4});
        wall3->transformComponent->SetRot(glm::vec3 {0, 0, 0.0});
        wall3->colliderComponent->elasticity = 1.0;
        wall3->transformComponent->SetScl({8, 8, 1});
        wall3->renderComponent->SetColor({1, 1, 1, 1});
        wall3->renderComponent->SetTextureZ(brickTextureZ);
        wall3->name = "wall";
        auto wall4 = ComponentRegistry::NewGameObject(params);
        wall4->transformComponent->SetPos({0, 4, -4});
        wall4->transformComponent->SetRot(glm::vec3 {0, 0, 0.0});
        wall4->colliderComponent->elasticity = 1.0;
        wall4->transformComponent->SetScl({8, 8, 1});
        wall4->renderComponent->SetColor({1, 1, 1, 1.0});
        wall4->renderComponent->SetTextureZ(brickTextureZ);
        wall4->name = "wall";
    }

    auto skyboxFaces = vector<std::string>(
    { 
        "../textures/sky/right.png",
        "../textures/sky/left.png",
        "../textures/sky/top.png",
        "../textures/sky/bottom.png",
        "../textures/sky/back.png",
        "../textures/sky/front.png"
    });

    {
        auto [index, sky_m_ptr] = Material::New({TextureCreateParams {skyboxFaces, Texture::ColorMap}}, Texture::TextureCubemap);
        GE.skyboxMaterial = sky_m_ptr;
        GE.skyboxMaterialLayer = index;
    }
    GE.debugFreecamPos = glm::vec3(0, 15, 0);
    
    
    // int nObjs = 0;
    // for (int x = 0; x < 1; x++) {
    //     for (int y = 0; y < 1; y++) {
    //         for (int z = 0; z < 1; z++) {
    //             GameobjectCreateParams params({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::RenderComponentBitIndex, ComponentRegistry::ColliderComponentBitIndex, ComponentRegistry::RigidbodyComponentBitIndex});
    //             params.meshId = m->meshId;
    //             params.materialId = brickMaterial->id;
    //             auto g = ComponentRegistry::NewGameObject(params);
    //             g->transformComponent->SetPos({x * 3,1.5 + y * 3, z * 3});
    //             g->colliderComponent->elasticity = 0.8;
    //             g->transformComponent->SetRot(glm::quat(glm::vec3(glm::radians(0.0), 0.0, glm::radians(0.0))));
    //             // g->rigidbodyComponent->velocity = {1.0, 0.0, 1.0};
    //             // g->rigidbodyComponent->angularVelocity = {1.0, 1.0, 1.0};
    //             g->transformComponent->SetScl(glm::dvec3(1.0, 1.0, 1.0));
    //             g->renderComponent->SetColor(glm::vec4(1, 1, 1, 1));
    //             g->renderComponent->SetTextureZ(brickTextureZ);
    //             g->name = std::string("Gameobject #") + to_string(nObjs);
    //             nObjs++;
    //         } 
    //     }
    // }

    
    // make light
    {
        GameobjectCreateParams params({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::PointlightComponentBitIndex, ComponentRegistry::RenderComponentBitIndex, ComponentRegistry::ColliderComponentBitIndex});
        params.meshId = m->meshId;
        params.materialId = 0;
        auto coolLight = ComponentRegistry::NewGameObject(params);
        coolLight->renderComponent->SetTextureZ(-1);
        coolLight->transformComponent->SetPos({5, 5, 5});
        coolLight->pointLightComponent->SetRange(200);
        coolLight->pointLightComponent->SetColor({1, 1, 1});
    }
    {
        GameobjectCreateParams params({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::PointlightComponentBitIndex, ComponentRegistry::RenderComponentBitIndex, ComponentRegistry::ColliderComponentBitIndex});
        params.meshId = m->meshId;
        params.materialId = grassMaterial->id;
        auto coolLight = ComponentRegistry::NewGameObject(params);
        coolLight->renderComponent->SetTextureZ(-1);
        coolLight->transformComponent->SetPos({40, 5, 40});
        coolLight->pointLightComponent->SetRange(1000);
        coolLight->pointLightComponent->SetColor({1, 1, 1});
    }

    // { // text rendering stuff
        auto ttfParams = TextureCreateParams({"../fonts/arial.ttf",}, Texture::FontMap);
        ttfParams.fontHeight = 60;
        ttfParams.format = Texture::Grayscale_8Bit;
        auto arialFont = Material::New({ttfParams, TextureCreateParams {{"../textures/ambientcg_bricks085/color.jpg",}, Texture::ColorMap}}, Texture::Texture2D);

        auto textMesh = Mesh::FromText(
            // "abcdefghijklmnopqrstuvwxyz\nABCDEFGHIJKLMNOPQRSTUVWXYZ",
            "Honey is a free browser add-on available on Google, Oprah, Firefox, Safari, if it's a browser it has Honey. All you have to do is when you're checking out on one of these major sites, just click that little orange button, and it will scan the entire internet and find discount codes for you. As you see right here, I'm on Hanes, y'know, ordering some shirts because who doesn't like ordering shirts; We saved 11 dollars! Dude our total is 55 dollars, and after Honey, it's 44 dollars. Boom. I clicked once and I saved 11 dollars. There's literally no reason not to install Honey. It takes two clicks, 10 million people use it, 100,000 five star reviews, unless you hate money, you should install Honey. ",
            arialFont.second->fontMapConstAccess.value());
        std::cout << "Textmesh id = "  << textMesh->meshId << ".\n";  
        GameobjectCreateParams params({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::RenderComponentBitIndex});
        params.meshId = textMesh->meshId;
        params.shaderId = ShaderProgram::New("../shaders/gui_vertex.glsl", "../shaders/gui_fragment.glsl")->shaderProgramId;
        params.materialId = arialFont.second->id;
        auto text = ComponentRegistry::NewGameObject(params);
        text->renderComponent->SetTextureZ(arialFont.first);
        text->transformComponent->SetPos({0, 15, 5.0});
        text->transformComponent->SetScl(textMesh->originalSize * 0.01f);
        text->transformComponent->SetRot(glm::quat(glm::vec3(0.0, 0.0, glm::radians(180.0))));

    // }
    
    
    GE.debugFreecamEnabled = true;
    GE.window.SetMouseLocked(true);
    
    glPointSize(8.0); // debug thing, ignore
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

    unsigned int accum = 0;
    std::string ttt = "a";
    while (!GE.ShouldClose()) 
    {
        accum += 1;
        if (accum == 180) {
            accum = 0;
            auto [vers, inds] = textMesh->StartModifying();
            TextMeshFromText(ttt, arialFont.second->fontMapConstAccess.value(), textMesh->vertexFormat, vers, inds);
            textMesh->StopModifying(false);
            text->transformComponent->SetScl(textMesh->originalSize * 0.01f);
            ttt += 'a';
        }
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
                    PE.Step(SIMULATION_TIMESTEP/2.0);
                    PE.Step(SIMULATION_TIMESTEP/4.0);
                    PE.Step(SIMULATION_TIMESTEP/8.0);
                    PE.Step(SIMULATION_TIMESTEP/8.0);
                }
            }
            
            physicsLag -= SIMULATION_TIMESTEP;
        }
        

        // printf("Doing a little raycasting.\n");
        if (LMB_DOWN) {
            auto castResult = Raycast(GE.debugFreecamPos, LookVector(glm::radians(GE.debugFreecamPitch), glm::radians(GE.debugFreecamYaw)));
            
            if (castResult.hitObject != nullptr) {
                // std::cout << "Hit object " << castResult.hitObject->name << ", normal is " << glm::to_string(castResult.hitNormal) << " \n";
            }

            if (castResult.hitObject != nullptr && castResult.hitObject->rigidbodyComponent) {
                
                castResult.hitObject->rigidbodyComponent->velocity += castResult.hitNormal * 0.4;
                //castResult.hitObject->transformComponent->SetPos(castResult.hitObject->transformComponent->position() + castResult.hitNormal * 0.02);
            }
            else {
                // std::cout << "LMB_DOWN but not hitting anything.\n";
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