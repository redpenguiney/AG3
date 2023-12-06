#include <cmath>
#include<cstdlib>
#include<cstdio>
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

using namespace std;

int main() {
    std::printf("Main function reached.\n");

    auto & GE = GraphicsEngine::Get();
    auto & PE = PhysicsEngine::Get();
    //GE.camera.position.y = 3;

    auto m = Mesh::FromFile("../models/rainbowcube.obj", true, true, -1.0, 1.0, 16384);
    auto [grassTextureZ, grassMaterial] = Material::New({TextureCreateParams {.texturePaths = {"../textures/grass.png"}, .format = RGB, .usage = ColorMap}, TextureCreateParams {.texturePaths = {"../textures/crate_specular.png"}, .format = Grayscale, .usage = SpecularMap}}, TEXTURE_2D);
    //std::printf("ITS %u %u\n", m->meshId, grassMaterial->id);
    auto [brickTextureZ, brickMaterial] = Material::New({
        TextureCreateParams {.texturePaths = {"../textures/ambientcg_bricks085/color.jpg"}, .format = RGBA, .usage = ColorMap}, 
        TextureCreateParams {.texturePaths = {"../textures/ambientcg_bricks085/roughness.jpg"}, .format = Grayscale, .usage = SpecularMap}, 
        TextureCreateParams {.texturePaths = {"../textures/ambientcg_bricks085/normal_gl.jpg"}, .format = RGB, .usage = NormalMap}, 
        TextureCreateParams {.texturePaths = {"../textures/ambientcg_bricks085/displacement.jpg"}, .format = Grayscale, .usage = DisplacementMap}
        }, TEXTURE_2D);


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
    GE.debugFreecamPos = glm::vec3(4, 4, 4);

    
    int i = 0;
    for (int x = -5; x < 2; x++) {
        for (int y = -5; y < 2; y++) {
            for (int z = -5; z < 2; z++) {
                GameobjectCreateParams params({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::RenderComponentBitIndex, ComponentRegistry::ColliderComponentBitIndex, ComponentRegistry::RigidbodyComponentBitIndex});
                params.meshId = m->meshId;
                params.materialId = brickMaterial->id;
                auto g = ComponentRegistry::NewGameObject(params);
                g->transformComponent->SetPos({x * 8, y * 8, z * 8});
                // g->transformComponent->SetRot(glm::quat(glm::vec3(1, 1, 0)));
                //g->transformComponent->SetScl(glm::dvec3(2, 2, 2));
                g->renderComponent->SetColor(glm::vec4(1, 1, 1, 1));
                g->renderComponent->SetTextureZ(brickTextureZ);
                g->name = std::string("Gameobject #") + to_string(i);
                i++;
            } 
        }
    }

    
    // make light
    {GameobjectCreateParams params({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::PointlightComponentBitIndex, ComponentRegistry::RenderComponentBitIndex, ComponentRegistry::ColliderComponentBitIndex, ComponentRegistry::RigidbodyComponentBitIndex});
    params.meshId = m->meshId;
    params.materialId = grassMaterial->id;
    auto coolLight = ComponentRegistry::NewGameObject(params);
    coolLight->renderComponent->SetTextureZ(-1);
    coolLight->transformComponent->SetPos({0, 10, 0});
    coolLight->pointLightComponent->SetRange(200);
    coolLight->pointLightComponent->SetColor({1, 1, 1});}
    {GameobjectCreateParams params({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::PointlightComponentBitIndex, ComponentRegistry::RenderComponentBitIndex, ComponentRegistry::ColliderComponentBitIndex, ComponentRegistry::RigidbodyComponentBitIndex});
    params.meshId = m->meshId;
    params.materialId = grassMaterial->id;
    auto coolLight = ComponentRegistry::NewGameObject(params);
    coolLight->renderComponent->SetTextureZ(-1);
    coolLight->transformComponent->SetPos({30, 10, 30});
    coolLight->pointLightComponent->SetRange(200);
    coolLight->pointLightComponent->SetColor({1, 1, 1});}
    
    
    GE.debugFreecamEnabled = true;
    GE.window.SetMouseLocked(true);
    
    glPointSize(4.0); // debug thing, ignore
    glLineWidth(4.0);

    //printf("Starting main loop.\n");

    while (!GE.ShouldClose()) 
    {
        //coolLight->transformComponent->SetPos({sin(1000) * 3, 2,  cos(1000) * 3});

        //GE.camera.position -= glm::dvec3(0.0001, -0.0001, 0.0);
        //auto start = Time();
        SpatialAccelerationStructure::Get().Update();
        
        PE.Step(1.0/60.0);

        if (LMB_DOWN) {
            auto castResult = Raycast(GE.debugFreecamPos, LookVector(glm::radians(GE.debugFreecamPitch), glm::radians(GE.debugFreecamYaw)));
            if (castResult.hitObject != nullptr && castResult.hitObject->rigidbodyComponent.ptr != nullptr) {
                //std::cout << "Hit object " << castResult.hitObject->name << " \n";
                castResult.hitObject->rigidbodyComponent->velocity += castResult.hitNormal * 0.02;
            }
            else {
                //std::cout << "LMB_DOWN but not hitting anything.\n";
            }

        }
        
        GE.RenderScene();
        //LogElapsed(start, "Frame elapsed");
        //GE.SetColor(drawId, glm::vec4(0.0, 1.0, 0.5, 1.0));
        //printf("FRAME SUCCESS");
    }
    printf("Beginning exit process.\n");

    auto start = Time();

    // It's very important that this function runs before GE, SAS, etc. are destroyed, and it's very important that other shared_ptrs to gameobjects outside of the GAMEOBJECTS map are gone (meaning lua needs to be deleted before this code runs)
    ComponentRegistry::CleanupComponents(); printf("Cleaned up all gameobjects.\n");

    LogElapsed(start, "Exit process elapsed ");
    printf("Program ran successfully (unless someone's destructor crashes after this print statement). Exiting.\n");
    return EXIT_SUCCESS;
}