#version 330
// #extension GL_ARB_shading_language_include : require
// #include "phong.glsl"


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
out vec3 cameraToFragmentPosition;
out vec3 fragmentNormal;
out vec3 fragmentTexCoords;
// out vec4 lightSpaceCoords;

void main()
{
    // TODO: we need to rotate normals i think?
    gl_Position = model * vec4(vertexPos, 1.0);
    cameraToFragmentPosition = gl_Position.xyz;
    gl_Position = camera * gl_Position;
    fragmentColor = vertexColor.xyz;
    fragmentNormal = vertexNormal;
    fragmentTexCoords = vec3(textureXY, textureZ);
    //lightSpaceCoords = modelToLightSpace * model * vec4(vertexPos, 1.0);
}          

