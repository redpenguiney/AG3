#version 330
// #extension GL_ARB_shading_language_include : require
// #include "phong.glsl"


layout(location=0) in vec2 vertexPos;
layout(location=1) in vec4 vertexColor;
layout(location=2) in vec2 textureXY;
layout(location=3) in float textureZ;

layout(location=6) in mat4 modelMatrix;
// locations 6-9 are part of model

layout(location=10) in mat3 normalMatrix;
// locations 10-12 are part of normalMatrix

// camera has projection matrix and camera matrix
uniform mat4 camera;
uniform mat4 modelToLightSpace;

out vec4 fragmentColor;
out vec3 fragmentTexCoords;
// out vec4 lightSpaceCoords;

void main()
{
    gl_Position = camera * modelMatrix * vec4(vertexPos, 1.0, 1.0);
    fragmentColor = vertexColor;
    fragmentTexCoords = vec3(textureXY, textureZ);
    //lightSpaceCoords = modelToLightSpace * model * vec4(vertexPos, 1.0);
}          

