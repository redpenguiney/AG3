#pragma once

#include <string>
#include <cstdio>
#include <fstream>
#include <GL/GL.h>
//#include <optional>

class ShaderProgram {
    public:
    bool useCameraMatrix; // if true the uniform mat4s "camera" and "proj" in this program's vertex shader will be automatically set

    ShaderProgram(char* vertexPath, char* fragmentPath) {

    }

    ~ShaderProgram() {

    }
};