#include<cstdlib>
#include<cstdio>
#include <memory>
#include <string>
#include <vector>

#include "graphics/engine.hpp"
#include "graphics/shader_program.hpp"
#include "graphics/texture.hpp"
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
    auto t = Texture::New(TEXTURE_2D_ARRAY, "../textures/grass.png");
    std::printf("ITS %u %u\n", m->meshId, t->textureId);

    auto skyboxFaces = vector<std::string>(
    {
        "../textures/sky/right.png",
        "../textures/sky/left.png",
        "../textures/sky/top.png",
        "../textures/sky/bottom.png",
        "../textures/sky/back.png",
        "../textures/sky/front.png"
    });
    GE.skyboxTexture = Texture::New(TEXTURE_CUBEMAP, skyboxFaces);
    GE.debugFreecamPos = glm::vec3(4, 4, 4);

    
    int i = 0;
    for (int x = 0; x < 50; x++) {
        for (int y = 0; y < 50; y++) {
            for (int z = 0; z < 50; z++) {
                CreateGameObjectParams params({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::RenderComponentBitIndex, ComponentRegistry::ColliderComponentBitIndex, ComponentRegistry::RigidbodyComponentBitIndex});
                params.meshId = m->meshId;
                params.textureId = t->textureId;
                auto g = ComponentRegistry::NewGameObject(params);
                g->transformComponent->SetPos({x * 6, y * 3 - 0, z * 3});
                //g->transformComponent->SetRot(glm::quat(glm::vec3(1, 1, 0)));
                //g->transformComponent->SetScl(glm::dvec3(1, 2, 1));
                g->renderComponent->SetColor(glm::vec4(i % 2, (i + 1) % 2, (i + 1) % 2, 1.0));
                g->renderComponent->SetTextureZ(0.0);
                g->name = std::string("Gameobject #") + to_string(i);
                i++;
            } 
        }
    }

    {
        // make light
        // CreateGameObjectParams params({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::PointlightComponentBitIndex});
        // auto g = ComponentRegistry::NewGameObject(params);
        // g->transformComponent->SetPos({0, 0, 0});
        // g->pointLightComponent->SetRange(100);
        // g->pointLightComponent->SetColor({1, 0, 0});
    }
    
    GE.debugFreecamEnabled = true;
    GE.window.SetMouseLocked(true);
    
    glPointSize(4.0); // debug thing, ignore

    printf("Starting main loop.\n");

    while (!GE.ShouldClose()) {

        //GE.camera.position -= glm::dvec3(0.0001, -0.0001, 0.0);
        //auto start = Time();
        SpatialAccelerationStructure::Get().Update();
        PE.Step(1);

        if (LMB_DOWN) {
            // auto castResult = Raycast(GE.debugFreecamPos, LookVector(glm::radians(GE.debugFreecamPitch), glm::radians(GE.debugFreecamYaw)));
            // if (castResult.hitObject != nullptr && castResult.hitObject->rigidbodyComponent.ptr != nullptr) {
            //     std::cout << "Hit object " << castResult.hitObject->name << " \n";
            //     castResult.hitObject->rigidbodyComponent->velocity += castResult.hitNormal * 0.1;
            // }
            // else {
            //     // thing->renderComponent->SetColor(glm::vec4(1, 1, 1, 1));
            // }

        }
        
        GE.RenderScene();
       // LogElapsed(start, "Drawing elapsed\n");
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