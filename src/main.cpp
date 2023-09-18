#include<cstdlib>
#include<cstdio>
#include <vector>

#include"graphics/engine.cpp"
#include"gameobjects/gameobject.cpp"

using namespace std;

int main() {
    //GE.camera.position.y = 3;

    // mesh.instanceCount/amount correctly drawn/amount i asked for
    // 4/12/40
    // 4/8/20
    // 2/10/40
    // 2/6/20
    // 1/10/40
    // 1/5/20

    auto m = Mesh::FromFile("../models/rainbowcube.obj", true, false, -1.0, 1.0, 8);
    
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
    
    int z = -100;
    while (!GE.ShouldClose()) {
        z++;
        for (int x = -50; x < 50; x++) {
            auto g = GameObject::New(m);
            g->transformComponent->position = glm::dvec3( x * 3, -1, z * 3);
        }

        //GE.camera.position -= glm::dvec3(0.0001, -0.0001, 0.0);
        GE.RenderScene();
        //GE.SetColor(drawId, glm::vec4(0.0, 1.0, 0.5, 1.0));
        //printf("FRAME SUCCESS");
    }
    printf("\nProgram ran successfully. Exiting.");
    return EXIT_SUCCESS;
}