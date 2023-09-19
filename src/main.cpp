#include<cstdlib>
#include<cstdio>
#include <vector>

#include "graphics/engine.cpp"
#include "graphics/shader_program.cpp"
#include "graphics/texture.cpp"
#include"gameobjects/gameobject.cpp"

using namespace std;

int main() {
    GraphicsEngine::Init();
    //GE.camera.position.y = 3;

    // mesh.instanceCount/amount correctly drawn/amount i asked for
    // 4/12/40
    // 4/8/20
    // 2/10/40
    // 2/6/20
    // 1/10/40
    // 1/5/20

    auto skyboxId = Texture::New(TEXTURE_CUBEMAP, {})

    auto m = Mesh::FromFile("../models/rainbowcube.obj", true, false, -1.0, 1.0, 16384);
    auto t = Texture::New(TEXTURE_2D_ARRAY, "../textures/grass.png");
    for (int x = -50; x < 10; x++) {
        for (int y = -50; y < 10; y++) {
            for (int z = 5; z < 10; z++) {
                auto g = GameObject::New(m, t);
                g->transformComponent->position = glm::dvec3( x * 3, y * 3, z * 3);
            }
        }
    }

    
    GraphicsEngine::debugFreecamEnabled = true;
    GraphicsEngine::window.SetMouseLocked(true);
    
    // std::printf("\n Indices: \n");
    // for (auto & i : Mesh::Get(m)->indices) {
    //     std::printf("%i ", i);
    // }
    // std::printf("\n Vertices: \n");
    // for (auto & v : Mesh::Get(m)->vertices) {
    //     std::printf("%f ", v);
    // }

    glPointSize(10.0); // debug thing ignore

    printf("\nStarting main loop.");
    
    while (!GraphicsEngine::ShouldClose()) {

        //GE.camera.position -= glm::dvec3(0.0001, -0.0001, 0.0);
        GraphicsEngine::RenderScene();
        //GE.SetColor(drawId, glm::vec4(0.0, 1.0, 0.5, 1.0));
        //printf("FRAME SUCCESS");
    }
    printf("\nBeginning exit process.");

    GraphicsEngine::Terminate();

    printf("\nProgram ran successfully. Exiting.");
    return EXIT_SUCCESS;
}