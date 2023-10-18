#include<cstdlib>
#include<cstdio>
#include <vector>

#include "graphics/engine.hpp"
#include "graphics/shader_program.hpp"
#include "graphics/texture.hpp"
#include "gameobjects/gameobject.hpp"

using namespace std;

int main() {
    std::printf("Main loop rehched.\n");
    auto & GE = GraphicsEngine::Get();
    //GE.camera.position.y = 3;

    // mesh.instanceCount/amount correctly drawn/amount i asked for
    // 4/12/40
    // 4/8/20
    // 2/10/40
    // 2/6/20
    // 1/10/40 
    // 1/5/20

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

    auto m = Mesh::FromFile("../models/rainbowcube.obj", true, false, -1.0, 1.0, 16384);
    auto t = Texture::New(TEXTURE_2D_ARRAY, "../textures/grass.png");
    
    int i = 0;
    for (int x = 0; x < 3; x++) {
        for (int y = 0; y < 1; y++) {
            for (int z = 0; z < 1; z++) {
                auto g = GameObject::New(m->meshId, t->textureId);
                g->transformComponent->SetPos({x * 3, y * 3, z * 3});
                //g->transformComponent->SetRot(glm::quat(glm::vec3(1, 1, 0)));
                //g->transformComponent->SetScl(glm::dvec3(1, 2, 1));
                //g->renderComponent->SetColor(glm::vec4(i % 2, (i + 1) % 2, (i + 1) % 2, 1.0));
                g->renderComponent->SetTextureZ(-1.0);
                i++;
            } 
        }
    }
    
    GE.debugFreecamEnabled = true;
    GE.window.SetMouseLocked(true);
    
    // std::printf("\n Indices: \n");
    // for (auto & i : Mesh::Get(m)->indices) {
    //     std::printf("%i ", i);
    // }
    // std::printf("\n Vertices: \n");
    // for (auto & v : Mesh::Get(m)->vertices) {
    //     std::printf("%f ", v);
    // }

    glPointSize(4.0); // debug thing, ignore

    printf("Starting main loop.\n");
    
    while (!GE.ShouldClose()) {
        //GE.camera.position -= glm::dvec3(0.0001, -0.0001, 0.0);
        //auto start = Time();
        SpatialAccelerationStructure::Get().Update();

        GE.RenderScene();
        //LogElapsed(start, "\nDrawing elapsed ");
        //GE.SetColor(drawId, glm::vec4(0.0, 1.0, 0.5, 1.0));
        //printf("FRAME SUCCESS");
    }
    printf("Beginning exit process.\n");

    auto start = Time();

    GameObject::Cleanup(); printf("Cleaned up all gameobjects.\n");

    LogElapsed(start, "Exit process elapsed ");
    printf("Program ran successfully. Exiting.\n");
    return EXIT_SUCCESS;
}