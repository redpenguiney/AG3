#version 430

in vec4 fragmentColor;
in vec3 fragmentNormal;
in vec3 fragmentTexCoords;
in vec3 cameraToFragmentPosition;
in vec3 cameraToFragmentInTangentSpace;
in mat3 TBNmatrix;
// in vec4 lightSpaceCoords;

#$INCLUDE$ "../shaders/phong_lighting.glsl"

layout(location = 0) out vec4 Output;

layout(binding=0) uniform sampler2DArray colorMap; // note: this syntax not ok until opengl 4.2
layout(binding=1) uniform sampler2DArray normalMap; // note: this syntax not ok until opengl 4.2
layout(binding=2) uniform sampler2DArray specularMap; // note: this syntax not ok until opengl 4.2
layout(binding=3) uniform sampler2DArray displacementMap; // note: this syntax not ok until opengl 4.2

uniform bool normalMappingEnabled;
uniform bool parallaxMappingEnabled;
uniform bool specularMappingEnabled;
uniform bool colorMappingEnabled;
uniform bool vertexColorEnabled; // could actually refer to vertex or instance color

#$INCLUDE$ "../shaders/parallax_mapping.glsl"



// TODO; to avoid color banding add dithering 
void main()
{
    vec3 realTexCoords;
    if (parallaxMappingEnabled) {
        realTexCoords = vec3(CalculateTexCoords(fragmentTexCoords).xy, fragmentTexCoords.z);       
        // if(realTexCoords.x > 1.0 || realTexCoords.y > 1.0 || realTexCoords.x < 0.0 || realTexCoords.y < 0.0) {
        //     discard;
        // }
    }
    else {    
        realTexCoords = fragmentTexCoords;// - vec3(0.5/1024.0, 0.5/1024.0, 0.0);
    }
    
    vec3 normal = normalize(fragmentNormal);
    if (normalMappingEnabled) {normal = normalize(TBNmatrix * (texture(normalMap, realTexCoords).rgb * 2.0 - 1.0));} // todo: matrix multiplication in fragment shader is really bad, maybe?

    float specularStrength = 0.5;
    if (specularMappingEnabled) {
        specularStrength = texture(specularMap, realTexCoords).x;
    }

    vec3 light = CalculateLighting(specularStrength, normal);

    vec4 tx;
    if (realTexCoords.z < 0) {
        tx = vec4(1.0, 1.0, 1.0, 1.0);
    }
    else {
        tx = texture(colorMap, realTexCoords);
    }
    

    //vec3 globalAmbient = vec3(0.1, 0.1, 0.1);

    vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
    if (vertexColorEnabled) {
        color = fragmentColor;
    }
    color *= tx * vec4(light, 1);

    if (color.a < 0.001) {
        //discard;
    };

    Output = color;
    //Output = vec4(1, 1, 1, 1);
};