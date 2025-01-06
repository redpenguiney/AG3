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
layout(location = 1) out float OutputAlpha;

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
    if (tx.a < 0.1) {
        discard;
    };

    //vec3 globalAmbient = vec3(0.1, 0.1, 0.1);

    vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
    if (vertexColorEnabled) {
        color = fragmentColor;
    }
    color *= tx * vec4(light, 1);

    // used for approximating OIT, see http://casual-effects.blogspot.com/2014/03/weighted-blended-order-independent.html 
    float asdfgh = min(1.0, max(max(color.r, color.g), color.b) * color.a);
    float weight = max(asdfgh, color.a) * clamp(0.03 / (1e-5 + pow(gl_FragCoord.z / 200, 4.0)), 1e-2, 3e3);
    weight = 1.0;
    Output = vec4(color.rgb * color.a, color.a) * weight;
    OutputAlpha = color.a;
    //Output = vec4(light, 1.0);
    //Output = vec4(spotLightOffset, 1.0, 1.0, 1.0);
    //Output = vec4(spotLights[0].directionAndOuterAngle.xyz, 1.0);
    //Output = vec4(normalize(fragmentNormal), 1.0);
    //Output = vec4(-envLightDirection, 1.0);
    //Output = vec4(fragmentColor.xyz, 1.0);
    //Output = vec4(gl_FragCoord.www, 1.0);
    //Output = vec4(dot(normal, envLightDirection));
    //Output = vec4(weight, weight, weight, color.a);
    //gl_FragDepth = 0.5;
};