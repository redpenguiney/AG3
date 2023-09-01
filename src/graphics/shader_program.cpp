#pragma once

#include <string>
#include <cstdio>
#include <fstream>
#include <GL/GL.h>
//#include <optional>

class Shader {
    Shader() {

    }
};

class ShaderProgram {
    public:
    GLuint programId;

    bool useCameraMatrix; // if true the uniform mat4s "camera" and "proj" in this program's vertex shader will be automatically set

    ShaderProgram(char* vertexPath, char* fragmentPath) {

    }

    ~ShaderProgram() {

    }

    private:
    GLuint fragmentId;
    GLuint vertexId;
};