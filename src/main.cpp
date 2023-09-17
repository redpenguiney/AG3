#include<cstdlib>
#include<cstdio>
#include <vector>

#include"graphics/engine.cpp"
#include"gameobjects/gameobject.cpp"

using namespace std;

int main() {
    //GE.camera.position.y = 3;
    auto m = Mesh::FromFile("../models/rainbowcube.obj", true, false, -1.0, 1.0, 2);
    for (int x = 0; x < 2; x++) {
        for (int y = 0; y < 2; y++) {
            for (int z = 0; z < 1; z++) {
                auto g = GameObject::New(m);
                g->transformComponent->position = glm::dvec3(x * 3, y * 3, z * 3);
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

    glPointSize(10.0); // debug thing ignore

    printf("\nStarting main loop.");
    
    while (!GE.ShouldClose()) {
        //GE.camera.position -= glm::dvec3(0.0001, -0.0001, 0.0);
        GE.RenderScene();
        //GE.SetColor(drawId, glm::vec4(0.0, 1.0, 0.5, 1.0));
        //printf("FRAME SUCCESS");
    }
    printf("\nProgram ran successfully. Exiting.");
    return EXIT_SUCCESS;
}