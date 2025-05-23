#version 330

layout(location=0) in vec3 vertexPos;
layout(location=1) in vec2 textureXY;
layout(location=2) in vec3 vertexNormal;

// camera has projection matrix and camera matrix
uniform mat4 perspective;

out vec3 fragmentTexCoords;

void main()
{
    vec4 position = perspective * vec4(vertexPos, 1.0);
    gl_Position = vec4(position.xyww); // this will make depth value be w/w = 1, so that skybox isn't drawn over stuff
    fragmentTexCoords = vertexPos;
}    