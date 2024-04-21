#version 330
// #extension GL_ARB_shading_language_include : require
// #include "phong.glsl"


layout(location=0) in vec3 vertexPos;
layout(location=1) in vec4 vertexColor;
layout(location=2) in vec2 textureXY;
layout(location=3) in float textureZ;

layout(location=6) in mat4 modelMatrix;
// locations 6-9 are part of model

layout(location=10) in mat3 normalMatrix;
// locations 10-12 are part of normalMatrix

// arb1 is z index
layout(location=13) in float arb1;

// (orthrographic projection)
uniform mat4 orthro;
uniform mat4 perspective;

out vec4 fragmentColor;
out vec3 fragmentTexCoords;

void main()
{

    gl_Position = perspective * modelMatrix * vec4(vertexPos, 1.0);
    gl_Position.z = arb1;
   
    fragmentColor = vertexColor;
    fragmentTexCoords = vec3(textureXY, textureZ);
}       
