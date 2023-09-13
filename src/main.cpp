#include<cstdlib>
#include<cstdio>
#include <vector>

#include"graphics/engine.cpp"
#include"gameobjects/gameobject.cpp"

using namespace std;

int main() {
    //GE.camera.position.y = 3;
    auto m = Mesh::FromFile("../models/rainbowcube.obj", true, false);
    auto g = GameObject::New(m);
    GE.debugFreecamEnabled = true;
    GE.window.SetMouseLocked(true);
    g->transformComponent->position = glm::dvec3(0.0, 0.0, -5.0);
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