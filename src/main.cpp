#include<cstdlib>
#include<cstdio>

#include"graphics/engine.cpp"
#include"gameobjects/gameobject.cpp"

using namespace std;

int main() {
    printf("\nStarting main loop.");
    while (!GE.ShouldClose()) {
        GE.RenderScene();
    }
    printf("\nProgram ran successfully. Exiting.");
    return EXIT_SUCCESS;
}