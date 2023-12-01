#version 430

in vec3 fragmentColor;
in vec3 fragmentNormal;
in vec3 fragmentTexCoords;
in vec3 cameraToFragmentPosition;
in mat3 TBNmatrix;
// in vec4 lightSpaceCoords;

layout(location = 0) out vec4 Output;

layout(binding=0) uniform sampler2DArray colorMap; // note: this syntax not ok until opengl 4.2
layout(binding=1) uniform sampler2DArray normalMap; // note: this syntax not ok until opengl 4.2
layout(binding=2) uniform sampler2DArray specularMap; // note: this syntax not ok until opengl 4.2
layout(binding=3) uniform sampler2DArray displacementMap; // note: this syntax not ok until opengl 4.2

uniform bool normalMappingEnabled;

struct pointLight {
    vec4 colorAndRange; // w-coord is range, xyz is rgb
    vec4 rel_pos; // w-coord is padding
};

layout(std430, binding = 1) buffer pointLightSSBO {
    uint pointLightCount;
    float stillPadding;
    float morePaddingLol;
    float alsoPadding;
    pointLight pointLights[];
};

vec3 CalculateLightInfluence(vec3 lightColor, vec3 rel_pos, float range, vec3 normal) {
    
    float distance = length(rel_pos - cameraToFragmentPosition);
    vec3 lightDir = normalize(rel_pos - cameraToFragmentPosition); 
    // float d = ;
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    float specularStrength = 0.8;
    vec3 viewDir = normalize(-cameraToFragmentPosition);
    vec3 reflectDir = reflect(-lightDir, normal); 
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  

    vec3 ambient = lightColor * 0.1;

    float strength = range/pow(distance, 2);
    return strength * (ambient + diffuse + (specular * texture(specularMap, fragmentTexCoords).x));
};

// TODO; to avoid color banding add dithering 
void main()
{
    vec3 normal = normalize(TBNmatrix * (texture(normalMap, fragmentTexCoords).rgb * 2.0 - 1.0)); // todo: matrix multiplication in fragment shader is really bad

    vec3 light = vec3(0, 0, 0);
    for (uint i = 0; i < pointLightCount; i++) {
        light += CalculateLightInfluence(pointLights[i].colorAndRange.xyz, pointLights[i].rel_pos.xyz, pointLights[i].colorAndRange.w, normal);
    }

    vec4 tx;
    if (fragmentTexCoords.z == -1.0) {
        tx = vec4(1.0, 1.0, 1.0, 1.0);
    }
    else {
        tx = texture(colorMap, fragmentTexCoords);
    }
    if (tx.a < 0.1) {
        discard;
    };
    vec4 color = tx * vec4(light * fragmentColor, 1);
    Output = color;

};