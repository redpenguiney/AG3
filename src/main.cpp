#include<cstdlib>
#include<cstdio>
#include <vector>

#include"graphics/engine.cpp"
#include"gameobjects/gameobject.cpp"

using namespace std;

std::vector<GLfloat> vertices = {0, 0, -0.5,   0, 0,   0, 0, 0};
std::vector<GLuint> indices = {0};

int main() {
    //auto m = Mesh::FromVertices(vertices, indices, true, true);
    auto m = Mesh::FromFile("../models/rainbowcube.obj", true, false);
    // std::printf("\n Indices: \n");
    // for (auto & i : Mesh::Get(m)->indices) {
    //     std::printf("%i ", i);
    // }
    // std::printf("\n Vertices: \n");
    // for (auto & v : Mesh::Get(m)->vertices) {
    //     std::printf("%f ", v);
    // }
    //auto drawId = GE.AddObject(m);
    
    glPointSize(10.0);
    printf("\nStarting main loop.");
    
    while (!GE.ShouldClose()) {
        GE.RenderScene();
        //GE.SetColor(drawId, glm::vec4(0.0, 1.0, 0.5, 1.0));
        //printf("FRAME SUCCESS");
    }
    printf("\nProgram ran successfully. Exiting.");
    return EXIT_SUCCESS;
}