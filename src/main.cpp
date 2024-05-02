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
    LogElapsed(timeAtWhichExitProcessStarted, "Exit process elapsed ");
    DebugLogInfo("Program ran successfully. Exiting.");
}

int main(int numArgs, const char *argPtrs[]) {
    

    DebugLogInfo("Main function reached.");

    atexit(AtExit);

    // TODO: shouldn't actually matter if these lines exist, and if it does fix that please
    auto & GE = GraphicsEngine::Get();
    auto & PE = PhysicsEngine::Get();
    auto & AE = AudioEngine::Get();
    auto & LUA = LuaHandler::Get();
    
    Module::LoadModule("..\\modules\\test_module.dll");

    //GE.camera.position.y = 3;

    // auto garticSound = Sound::New("../sounds/garticphone.wav");
    // {
    //     auto params = GameobjectCreateParams({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::AudioPlayerComponentBitIndex});
    //     params.sound = garticSound;
    //     auto soundSounder = ComponentRegistry::NewGameObject(params);
    //     soundSounder->audioPlayerComponent->looped = true;
    //     soundSounder->audioPlayerComponent->positional = true;
    //     soundSounder->audioPlayerComponent->volume = 0.15;
    //     soundSounder->audioPlayerComponent->pitch = 0.5;
    //     soundSounder->audioPlayerComponent->Play();
    // }

    auto m = Mesh::FromFile("../models/rainbowcube.obj", MeshVertexFormat::Default(), -1.0, 1.0, 16384);
    
    // LUA.RunString("print(\"help from lua\")");
    // LUA.RunFile("../scripts/test.lua");

    auto [grassTextureZ, grassMaterial] = Material::New({TextureCreateParams {{"../textures/grass.png",}, Texture::ColorMap}, TextureCreateParams {{"../textures/crate_specular.png",}, Texture::SpecularMap}}, Texture::Texture2D);

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
        floor->transformComponent->SetRot(glm::vec3 {0.0, 0, glm::radians(0.0)});
        floor->colliderComponent->elasticity = 0.9;
        floor->transformComponent->SetScl({10, 1, 10});
        floor->renderComponent->SetColor({0, 1, 0, 1.0});
        floor->renderComponent->SetTextureZ(grassTextureZ);
        floor->name = "ah yes the floor here is made of floor";
    }

    // GameobjectCreateParams wallParams({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::RenderComponentBitIndex, ComponentRegistry::ColliderComponentBitIndex});
    //     wallParams.meshId = m->meshId;
    //     wallParams.materialId = brickMaterial->id;
    //     auto wall1 = ComponentRegistry::NewGameObject(wallParams);
    //     wall1->transformComponent->SetPos({4, 4, 0});
    //     wall1->transformComponent->SetRot(glm::vec3 {0, 0, 0.0});
    //     wall1->colliderComponent->elasticity = 1.0;
    //     wall1->transformComponent->SetScl({1, 8, 8});
    //     wall1->renderComponent->SetColor({0, 1, 1, 1});
    //     wall1->renderComponent->SetTextureZ(brickTextureZ);
    //     wall1->name = "wall";
    // {
        
    //     auto wall2 = ComponentRegistry::NewGameObject(wallParams);
    //     wall2->transformComponent->SetPos({-4, 4, 0});
    //     wall2->transformComponent->SetRot(glm::vec3 {0, 0, 0.0});
    //     wall2->colliderComponent->elasticity = 1.0;
    //     wall2->transformComponent->SetScl({1, 8, 8});
    //     wall2->renderComponent->SetColor({1, 1, 1, 1.0});
    //     wall2->renderComponent->SetTextureZ(brickTextureZ);
    //     wall2->name = "wall";
    //     auto wall3 = ComponentRegistry::NewGameObject(wallParams);
    //     wall3->transformComponent->SetPos({0, 4, 4});
    //     wall3->transformComponent->SetRot(glm::vec3 {0, 0, 0.0});
    //     wall3->colliderComponent->elasticity = 1.0;
    //     wall3->transformComponent->SetScl({8, 8, 1});
    //     wall3->renderComponent->SetColor({1, 1, 1, 1});
    //     wall3->renderComponent->SetTextureZ(brickTextureZ);
    //     wall3->name = "wall";
    //     auto wall4 = ComponentRegistry::NewGameObject(wallParams);
    //     wall4->transformComponent->SetPos({0, 4, -4});
    //     wall4->transformComponent->SetRot(glm::vec3 {0, 0, 0.0});
    //     wall4->colliderComponent->elasticity = 1.0;
    //     wall4->transformComponent->SetScl({8, 8, 1});
    //     wall4->renderComponent->SetColor({1, 1, 1, 1.0});
    //     wall4->renderComponent->SetTextureZ(brickTextureZ);
    //     wall4->name = "wall";

    // //     wall2->transformComponent->SetParent(*wall1->transformComponent);
    // //     wall3->transformComponent->SetParent(*wall2->transformComponent);
    // //     wall4->transformComponent->SetParent(*wall3->transformComponent);

    // //     // wall1->transformComponent->SetRot(wall1->transformComponent->Rotation() * glm::quat(glm::vec3(0.0, glm::radians(15.0), 0.0)));
    // //     // wall1->transformComponent->SetScl({1, 1, 3});
    // }

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
    GE.debugFreecamCamera.position = glm::dvec3(0, 15, 0);
    
    std::weak_ptr<GameObject> goWeakPtr;
    int nObjs = 0;
    for (int x = 0; x < 1; x++) {
        for (int y = 0; y < 1; y++) {
            for (int z = 0; z < 1; z++) {
                GameobjectCreateParams params({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::RenderComponentBitIndex, ComponentRegistry::ColliderComponentBitIndex, ComponentRegistry::RigidbodyComponentBitIndex});
                params.meshId = m->meshId;
                params.materialId = brickMaterial->id;
                auto g = ComponentRegistry::NewGameObject(params);
                g->transformComponent->SetPos({0 + x * 3,3.0 + y * 3, 0 + z * 3});
                g->colliderComponent->elasticity = 0.0;
                g->transformComponent->SetRot(glm::quat(glm::vec3(glm::radians(0.0), glm::radians(0.0), glm::radians(0.0))));
                // g->rigidbodyComponent->velocity = {1.0, 0.0, 1.0};
                // g->rigidbodyComponent->angularVelocity = {1.0, 1.0, 1.0};
                g->transformComponent->SetScl(glm::dvec3(1.0, 1.0, 1.0));
                g->renderComponent->SetColor(glm::vec4(1, 1, 1, 1));
                g->renderComponent->SetTextureZ(brickTextureZ);
                g->name = std::string("Gameobject #") + to_string(nObjs);
                goWeakPtr = g;
                nObjs++;
            } 
        }
    }

    
    // // make light
    {
        GameobjectCreateParams params({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::PointlightComponentBitIndex, ComponentRegistry::RenderComponentBitIndex, ComponentRegistry::ColliderComponentBitIndex});
        params.meshId = m->meshId;
        params.materialId = 0;
        auto coolLight = ComponentRegistry::NewGameObject(params);
        coolLight->renderComponent->SetTextureZ(-1);
        coolLight->transformComponent->SetPos({8, 5, 0});
        coolLight->pointLightComponent->SetRange(200);
        coolLight->pointLightComponent->SetColor({1, 1, 1});
    }
    // {
    //     GameobjectCreateParams params({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::PointlightComponentBitIndex, ComponentRegistry::RenderComponentBitIndex, ComponentRegistry::ColliderComponentBitIndex});
    //     params.meshId = m->meshId;
    //     params.materialId = grassMaterial->id;
    //     auto coolLight = ComponentRegistry::NewGameObject(params);
    //     coolLight->renderComponent->SetTextureZ(-1);
    //     coolLight->transformComponent->SetPos({40, 5, 40});
    //     coolLight->pointLightComponent->SetRange(1000);
    //     coolLight->pointLightComponent->SetColor({1, 1, 1});
    // }

    { // text rendering stuff
        // auto ttfParams = TextureCreateParams({"../fonts/arial.ttf",}, Texture::FontMap);
        // ttfParams.fontHeight = 60;
        // ttfParams.format = Texture::Grayscale_8Bit;
        // auto [arialLayer, arialFont] = Material::New({ttfParams}, Texture::Texture2D);
        // auto textMesh = Mesh::FromText(
        //     // "abcdefghijklmnopqrstuvwxyz\nABCDEFGHIJKLMNOPQRSTUVWXYZ",
        //     "Honey is a free browser add-on available on Google, Oprah, Firefox, Safari, if it's a browser it has Honey. All you have to do is when you're checking out on one of these major sites, just click that little orange button, and it will scan the entire internet and find discount codes for you. As you see right here, I'm on Hanes, y'know, ordering some shirts because who doesn't like ordering shirts; We saved 11 dollars! Dude our total is 55 dollars, and after Honey, it's 44 dollars. Boom. I clicked once and I saved 11 dollars. There's literally no reason not to install Honey. It takes two clicks, 10 million people use it, 100,000 five star reviews, unless you hate money, you should install Honey. ",
        //     arialFont->fontMapConstAccess.value());
        // std::cout << "Textmesh id = "  << textMesh->meshId << ".\n";  
        // GameobjectCreateParams params({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::RenderComponentBitIndex});
        // params.meshId = textMesh->meshId;
        // params.shaderId = GE.defaultGuiShaderProgram->shaderProgramId;
        // params.materialId = arialFont->id;
        // auto text = ComponentRegistry::NewGameObject(params);
        // text->renderComponent->SetTextureZ(arialLayer);
        // text->transformComponent->SetPos({0, 0, 0});
        // text->transformComponent->SetScl(textMesh->originalSize * 0.01f);
        // text->transformComponent->SetRot(glm::quat(glm::vec3(0.0, 0.0, glm::radians(180.0))));

    }
    
    
    GE.debugFreecamEnabled = true;
    GE.window.SetMouseLocked(true);
    
    glPointSize(8.0); // debug thing, ignore
    glLineWidth(2.0);

    Gui* ui;
    {
        auto ttfParams = TextureCreateParams({"../fonts/arial.ttf",}, Texture::FontMap);
        ttfParams.fontHeight = 16;
        ttfParams.format = Texture::Grayscale_8Bit;
        auto [arialLayer, arialFont] = Material::New({ttfParams}, Texture::Texture2D, true);

        ui = new Gui(true, 
        std::make_optional(std::make_pair(arialLayer, arialFont)), 
        std::nullopt, 
        Gui::BillboardGuiInfo({.scaleWithDistance = false, .rotation = std::nullopt, .followObject = goWeakPtr}), 
        GraphicsEngine::Get().defaultBillboardGuiShaderProgram);
        ui->scaleSize = {0.5, 0.15};
        ui->guiScaleMode = Gui::ScaleXX;
        ui->offsetPos = {0.0, 0.0};
        // ui->scalePos = {0.5, 0.5};
        ui->anchorPoint = {0.0, 0.0};
        ui->rgba.a = 0.0;
        ui->GetTextInfo().rgba = {1.0, 1.0, 1.0, 1.0};

        // ui->GetTextInfo().text = "Honey is a free browser add-on available on Google, Oprah, Firefox, Safari, if it's a browser it has Honey. All you have to do is when you're checking out on one of these major sites, just click that little orange button, and it will scan the entire internet and find discount codes for you. As you see right here, I'm on Hanes, y'know, ordering some shirts because who doesn't like ordering shirts; We saved 11 dollars! Dude our total is 55 dollars, and after Honey, it's 44 dollars. Boom. I clicked once and I saved 11 dollars. There's literally no reason not to install Honey. It takes two clicks, 10 million people use it, 100,000 five star reviews, unless you hate money, you should install Honey. ";
        // ui->GetTextInfo().text = "Tga appbHb kok wjijj wa abcdefghijk eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";
        ui->GetTextInfo().topMargin = 0;
        ui->GetTextInfo().bottomMargin = 0;
        ui->GetTextInfo().lineHeight = 1.0;
        ui->GetTextInfo().horizontalAlignment = HorizontalAlignMode::Center;
        ui->GetTextInfo().verticalAlignment = VerticalAlignMode::Center;
        ui->UpdateGuiText();
        ui->UpdateGuiGraphics();
        ui->UpdateGuiTransform();
        
        // Gui ui2(false, std::make_optional(std::make_pair(arialLayer, arialFont)));
        // ui2.scaleSize = {0.25, 0.05};
        // ui2.guiScaleMode = Gui::ScaleXX;
        // ui2.offsetPos = {0.0, 0.0};
        // ui2.scalePos = {0.75, 0.0};
        // ui2.anchorPoint = {0.0, -1.0};   

        // ui2.rgba = {1.0, 0.5, 0.0, 1.0};
        // ui2.UpdateGuiGraphics();
        // ui2.UpdateGuiTransform();
    }

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
        
        ui->GetTextInfo().text = glm::to_string(goWeakPtr.lock()->rigidbodyComponent->velocity);
        ui->UpdateGuiText();

        // printf("Doing a little raycasting.\n");
        if (LMB_DOWN) {
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


        if (PRESS_BEGAN_KEYS[GLFW_KEY_SPACE]) {
            physicsPaused = !physicsPaused;
        }
        if (PRESS_BEGAN_KEYS[GLFW_KEY_ESCAPE]) {
            GE.window.Close();
        }
        if (PRESS_BEGAN_KEYS[GLFW_KEY_TAB]) {
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

        // if (g->transformComponent->position.y <= 1.0) {
        //     while (true) {}
        // }

        // std::printf("ok %f %f \n", GE.debugFreecamPitch, GE.debugFreecamYaw);
        // GE.window.Close();

        // LogElapsed(currentTime, "Frame elapsed");

    }

    delete ui;

    timeAtWhichExitProcessStarted = Time();
    DebugLogInfo("Beginning exit process.");

    // It's very important that this function runs before GE, SAS, etc. are destroyed, and it's very important that other shared_ptrs to gameobjects outside of the GAMEOBJECTS map are gone (meaning lua needs to be deleted before this code runs) so that all gameobjects are destructed before the things they need to destruct themselves (GE, PE, etc.) are destructed
    // TODO: register at exit instead?
    ComponentRegistry::CleanupComponents(); 
    DebugLogInfo("Cleaned up all gameobjects.");
    return EXIT_SUCCESS;
}