#include<cstdlib>
#include<cstdio>
#include <memory>
#include <string>
#include <vector>

#include "graphics/engine.hpp"
#include "graphics/shader_program.hpp"
#include "graphics/texture.hpp"
#include "gameobjects/gameobject.hpp"
#include "physics/raycast.hpp"

using namespace std;

int main() {
    std::printf("Main function reached.\n");

    auto & GE = GraphicsEngine::Get();
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

    
    std::shared_ptr<GameObject> thing;
    int i = 0;
    for (int x = 0; x < 10; x++) {
        for (int y = 0; y < 10; y++) {
            for (int z = 0; z < 10; z++) {
                CreateGameObjectParams params;
                params.haveGraphics = true;
                params.meshId = m->meshId;
                params.textureId = t->textureId;
                params.haveCollisions = true;
                std::printf("%u %uawww\n", params.meshId, params.textureId);
                auto g = GameObject::New(params);
                thing = g;
                g->transformComponent->SetPos({x * 6, y * 3 - 0, z * 3});
                //g->transformComponent->SetRot(glm::quat(glm::vec3(1, 1, 0)));
                //g->transformComponent->SetScl(glm::dvec3(1, 2, 1));
                //g->renderComponent->SetColor(glm::vec4(i % 2, (i + 1) % 2, (i + 1) % 2, 1.0));
                g->renderComponent->SetTextureZ(0.0);
                g->name = std::string("Gameobject #") + to_string(x);
                i++;
            } 
        }
    }

    {
        // make light
        CreateGameObjectParams params;
        params.havePointLight = true;
        params.haveGraphics = false;
        auto g = GameObject::New(params);
        g->transformComponent->SetPos({0, 0, 0});
        g->pointLightComponent->SetRange(100);
        g->pointLightComponent->SetColor({1, 0, 0});
    }
    
    GE.debugFreecamEnabled = true;
    GE.window.SetMouseLocked(true);
    
    glPointSize(4.0); // debug thing, ignore

    printf("Starting main loop.\n");

    while (!GE.ShouldClose()) {

        //GE.camera.position -= glm::dvec3(0.0001, -0.0001, 0.0);
        //auto start = Time();
        SpatialAccelerationStructure::Get().Update();

        auto castResult = Raycast(GE.debugFreecamPos, LookVector(glm::radians(GE.debugFreecamPitch), glm::radians(GE.debugFreecamYaw)));
        // if (castResult.hitObject != nullptr) {
        //     thing->renderComponent->SetColor(glm::vec4(1, 0, 0, 1));
        //     thing->transformComponent->SetPos(thing->transformComponent->position() + castResult.hitNormal * 0.01);
        // }
        // else {
        //     thing->renderComponent->SetColor(glm::vec4(1, 1, 1, 1));
        // }

        GE.RenderScene();
       // LogElapsed(start, "Drawing elapsed\n");
        //GE.SetColor(drawId, glm::vec4(0.0, 1.0, 0.5, 1.0));
        //printf("FRAME SUCCESS");
    }
    printf("Beginning exit process.\n");

    auto start = Time();

    GameObject::Cleanup(); printf("Cleaned up all gameobjects.\n");

    LogElapsed(start, "Exit process elapsed ");
    printf("Program ran successfully (unless someone's destructor crashes after this print statement). Exiting.\n");
    return EXIT_SUCCESS;
}