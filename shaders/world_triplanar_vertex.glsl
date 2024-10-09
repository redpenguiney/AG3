#version 330
// #extension GL_ARB_shading_language_include : require

layout(location=0) in vec3 vertexPos;
layout(location=1) in vec4 vertexColor;
layout(location=4) in vec3 vertexNormal;

layout(location=6) in mat4 modelMatrix;
// locations 6-9 are part of model

layout(location=10) in mat3 normalMatrix;
// locations 10-12 are part of normalMatrix

// perspective has projection matrix and camera matrix
uniform mat4 perspective;
uniform mat4 modelToLightSpace;

uniform bool normalMappingEnabled;

out vec4 fragmentColor;
out vec3 cameraToFragmentPosition;
out vec3 fragmentNormal;
//out mat3 TBNmatrix; //TBN matrix is need to make normal mapping work when an object is rotated
// out vec4 lightSpaceCoords;

void main()
{
    gl_Position = modelMatrix * vec4(vertexPos, 1.0);
    cameraToFragmentPosition = gl_Position.xyz;
    gl_Position = perspective * gl_Position;
    fragmentColor = vertexColor;
    fragmentNormal = normalize(normalMatrix * vertexNormal);
    //lightSpaceCoords = modelToLightSpace * model * vec4(vertexPos, 1.0);

    //vec3 T = normalize(vec3(modelMatrix * vec4(aTangent,   0.0)));
    //vec3 B = cross(fragmentNormal, T);
    //TBNmatrix = mat3(T, B, fragmentNormal);
}          

