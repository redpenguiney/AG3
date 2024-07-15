#version 430 // TODO: version too high
// #extension GL_ARB_shading_language_include : require
// #include "phong.glsl"


layout(location=0) in vec3 vertexPos;
layout(location=1) in vec4 vertexColor;
layout(location=2) in vec2 textureXY;
layout(location=3) in float textureZ;
layout(location=4) in vec3 vertexNormal;
layout(location=5) in vec3 aTangent;

layout(location=6) in mat4 modelMatrix;
// locations 6-9 are part of model

layout(location=10) in mat3 normalMatrix;
// locations 10-12 are part of normalMatrix

// 13 and 14 are arbitrary1 and 2
layout(location = 13) in ivec4 boneIds;
layout(location = 14) in vec4 weights;

// perspective has projection matrix and camera matrix
uniform mat4 perspective;
uniform mat4 modelToLightSpace;

layout(std140, binding = 1) readonly buffer boneSsbo {
    mat4 finalBonesMatrices[];
};
layout(std140, binding = 2) readonly buffer boneOffsetSsbo {
    uint boneBufferOffsets[];
};

uniform bool normalMappingEnabled;

out vec4 fragmentColor;
out vec3 cameraToFragmentPosition;
out vec3 cameraToFragmentInTangentSpace;
out vec3 fragmentNormal;
out vec3 fragmentTexCoords;
out mat3 TBNmatrix; //TBN matrix is need to make normal mapping work when an object is rotated
// out vec4 lightSpaceCoords;

void main()
{
    uint offset = 0; //boneBufferOffsets[(boneIds.x >> 16) + gl_InstanceID];
    ivec4 realBoneIds = boneIds;
    // realBoneIds.x = realBoneIds.x & 0x0000FFFF;
    vec4 totalPosition = vec4(0.0f);
    for(int i = 0 ; i < 4 ; i++)
    {
        if(realBoneIds[i] == -1) 
            continue;
        if(realBoneIds[i] >= boneBufferOffsets.length()) 
        {
            totalPosition = vec4(vertexPos,1.0f);
            break;
        }
        //vec4 localPosition = finalBonesMatrices[boneIds[i] + offset] * vec4(vertexPos,1.0f);
        mat4 m;
        m[1] = vec4(1);
        m[2] = vec4(3);
        vec4 localPosition = m * vec4(vertexPos, 1.0f);
        totalPosition += localPosition * weights[i];
        // vec3 localNormal = mat3(finalBonesMatrices[boneIds[i] + offset]) * norm;
    }

    gl_Position = modelMatrix * totalPosition;
    cameraToFragmentPosition = gl_Position.xyz;
    gl_Position = perspective * gl_Position;
    //fragmentColor = vec4(weights.xyz, 1);
    fragmentColor = vertexColor;
    fragmentNormal = normalize(normalMatrix * vertexNormal);
    fragmentTexCoords = vec3(textureXY, textureZ);
    //lightSpaceCoords = modelToLightSpace * model * vec4(vertexPos, 1.0);

    vec3 T = normalize(vec3(modelMatrix * vec4(aTangent,   0.0)));
    vec3 B = cross(fragmentNormal, T);
    TBNmatrix = mat3(T, B, fragmentNormal);
    cameraToFragmentInTangentSpace = TBNmatrix * (cameraToFragmentPosition);
}          

