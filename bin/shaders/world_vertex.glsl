#version 460 // TODO: DON'T USE THIS SHADER VERSION

layout(location=0) in vec3 vertexPos;
layout(location=1) in vec4 vertexColor;
layout(location=2) in vec2 textureXY;
layout(location=3) in float textureZ;
layout(location=4) in vec3 vertexNormal;

layout(location=5) in mat4 model;
// locations 5-7 are part of model

// camera has projection matrix and camera matrix
uniform mat4 camera;
uniform mat4 modelToLightSpace;

out vec3 fragmentColor;
out vec3 fragmentNormal;
out vec3 fragmentTexCoords;
out vec4 lightSpaceCoords;

void main()
{
    gl_Position = camera * model * vec4(vertexPos, 1.0);
    fragmentColor = vertexColor.xyz;
    fragmentNormal = vertexNormal;
    fragmentTexCoords = vec3(textureXY, textureZ);
    //lightSpaceCoords = modelToLightSpace * model * vec4(vertexPos, 1.0);
}          

