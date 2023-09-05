#include<cstdlib>
#include<cstdio>
#include <vector>

#include"graphics/engine.cpp"
#include"gameobjects/gameobject.cpp"

using namespace std;

std::vector<GLfloat> vertices = {0, 0, 0,   0, 0,   0, 0, 0};
std::vector<GLuint> indices = {0};

int main() {
    //auto m = Mesh::FromVertices(vertices, indices, true, true);
    auto m = Mesh::FromFile("../models/rainbowcube.obj");
    GE.AddObject(m);

    glPointSize(10.0);
    printf("\nStarting main loop.");
    
    while (!GE.ShouldClose()) {
        GE.RenderScene();
        //printf("FRAME SUCCESS");
    }
    printf("\nProgram ran successfully. Exiting.");
    return EXIT_SUCCESS;
}