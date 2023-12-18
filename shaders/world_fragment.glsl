#version 430

in vec3 fragmentColor;
in vec3 fragmentNormal;
in vec3 fragmentTexCoords;
in vec3 cameraToFragmentPosition;
in vec3 cameraToFragmentInTangentSpace;
in mat3 TBNmatrix;
// in vec4 lightSpaceCoords;

layout(location = 0) out vec4 Output;

layout(binding=0) uniform sampler2DArray colorMap; // note: this syntax not ok until opengl 4.2
layout(binding=1) uniform sampler2DArray normalMap; // note: this syntax not ok until opengl 4.2
layout(binding=2) uniform sampler2DArray specularMap; // note: this syntax not ok until opengl 4.2
layout(binding=3) uniform sampler2DArray displacementMap; // note: this syntax not ok until opengl 4.2

uniform bool normalMappingEnabled;
uniform bool parallaxMappingEnabled;

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

// parallax mapping
vec2 CalculateTexCoords(vec3 texCoords) { 
    vec3 viewDir = -normalize(cameraToFragmentInTangentSpace);

    float height =  texture(displacementMap, texCoords).r;    
    vec2 p = viewDir.xy / viewDir.z * (height * 0.1);
    return texCoords.xy + p;   

    // // number of depth layers
    // const float minLayers = 8;
    // const float maxLayers = 32;
    
    // // TODO: make uniform
    // const float heightScale = 4;

    // float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));  
    // // calculate the size of each layer
    // float layerDepth = 1.0 / numLayers;
    // // depth of current layer
    // float currentLayerDepth = 0.0;
    // // the amount to shift the texture coordinates per layer (from vector P)
    // vec2 P = viewDir.xy / viewDir.z * heightScale; 
    // vec2 deltaTexCoords = P / numLayers;
  
    // // get initial values
    // vec2  currentTexCoords = texCoords.xy;
    // float texZ = texCoords.z;
    // float currentDepthMapValue = texture(displacementMap, vec3(currentTexCoords, texZ)).r;
      
    // while(currentLayerDepth < currentDepthMapValue)
    // {
    //     // shift texture coordinates along direction of P
    //     currentTexCoords -= deltaTexCoords;
    //     // get depthmap value at current texture coordinates
    //     currentDepthMapValue = texture(displacementMap, vec3(currentTexCoords, texZ)).r;  
    //     // get depth of next layer
    //     currentLayerDepth += layerDepth;  
    // }
    
    // // get texture coordinates before collision (reverse operations)
    // vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // // get depth after and before collision for linear interpolation
    // float afterDepth  = currentDepthMapValue - currentLayerDepth;
    // float beforeDepth = texture(displacementMap, vec3(prevTexCoords, texZ)).r - currentLayerDepth + layerDepth;
 
    // // interpolation of texture coordinates
    // float weight = afterDepth / (afterDepth - beforeDepth);
    // vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    // return finalTexCoords;
}


vec3 CalculateLightInfluence(vec3 lightColor, vec3 rel_pos, float range, vec3 normal, vec3 realTexCoords) {
    
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
    return strength * (ambient + diffuse + (specular * texture(specularMap, realTexCoords).x));
};

// TODO; to avoid color banding add dithering 
void main()
{
    vec3 realTexCoords;
    if (parallaxMappingEnabled) {
        realTexCoords = vec3(CalculateTexCoords(fragmentTexCoords).xy, fragmentTexCoords.z);       
        // if(realTexCoords.x > 1.0 || realTexCoords.y > 1.0 || realTexCoords.x < 0.0 || realTexCoords.y < 0.0) {
        //    discard;
        // }
    }
    else {    
        realTexCoords = fragmentTexCoords;
    }
    
    vec3 normal = fragmentNormal;
    if (normalMappingEnabled) {normal = normalize(TBNmatrix * (texture(normalMap, realTexCoords).rgb * 2.0 - 1.0));} // todo: matrix multiplication in fragment shader is really bad

    vec3 light = vec3(1, 1, 1);
    //for (uint i = 0; i < pointLightCount; i++) {
    //    light += CalculateLightInfluence(pointLights[i].colorAndRange.xyz, pointLights[i].rel_pos.xyz, pointLights[i].colorAndRange.w, normal, realTexCoords);
    //}

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
    vec4 color = tx * vec4(light * fragmentColor, 1);
    Output = color;

};