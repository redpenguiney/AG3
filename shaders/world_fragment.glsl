#version 430

in vec3 fragmentColor;
in vec3 fragmentNormal;
in vec3 fragmentTexCoords;
in vec4 lightSpaceCoords;

layout(location = 0) out vec4 Output;

uniform sampler2DArray textures;

struct pointLight {
    vec4 colorAndRange; // w-coord is range, xyz is rgb
    vec4 rel_pos; // w-coord is padding
};

layout(std430, binding = 0) buffer pointLightSSBO {
    uint pointLightCount;
    vec3 morePaddingLol;
    pointLight pointLights[];
};

vec3 CalculateLightInfluence(vec3 color, vec3 rel_pos, float range) {
    //vec3 norm = normalize(fragmentNormal);
    //vec3 lightDir = normalize(rel_pos); 
    //float diff = max(dot(norm, lightDir), 0.0);
    //vec3 diffuse = diff * color;

    vec3 ambient = color * 0.5;
    return ambient;
};


void main()
{
    vec3 light = vec3(0, 0, 0);
    for (uint i = 0; i < pointLightCount; i++) {
        light = CalculateLightInfluence(pointLights[i].colorAndRange.xyz, pointLights[i].rel_pos.xyz, pointLights[i].colorAndRange.w);
    }

    vec4 tx;
    if (fragmentTexCoords.z == -1.0) {
        tx = vec4(1.0, 1.0, 1.0, 1.0);
    }
    else {
        tx = texture(textures, fragmentTexCoords);
    }
    if (tx.a < 0.1) {
        discard;
    };
    vec4 color = tx * vec4(fragmentColor, 1);
    Output = vec4(light, color.w);

};