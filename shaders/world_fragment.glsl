#version 430

in vec4 fragmentColor;
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
uniform bool specularMappingEnabled;
uniform bool colorMappingEnabled;

uniform vec3 envLightDirection;
uniform vec3 envLightColor;
uniform float envLightDiffuse;
uniform float envLightAmbient;
uniform float envLightSpecular;

struct pointLight {
    vec4 colorAndRange; // w-coord is range, xyz is rgb
    vec4 rel_pos; // w-coord is padding
};

layout(std430, binding = 0) buffer pointLightSSBO {
    uint pointLightCount;
    float stillPadding;
    float morePaddingLol;
    float alsoPadding;
    pointLight pointLights[];
};

struct spotLight {
    vec4 colorAndRange; // w-coord is range, xyz is rgb
    vec4 relPosAndInnerAngle; // w-coord is cos(inner angle)
    vec4 directionAndOuterAngle; // w-coord is cos(outer angle)
};

layout(std430, binding = 0) buffer pointLightSSBO {
    uint spotLightCount;
    float stillPadding2;
    float morePaddingLol2;
    float alsoPadding2;
    spotLight spotLights[];
};

// parallax mapping
vec2 CalculateTexCoords(vec3 texCoords) { 
    vec3 viewDir = normalize(cameraToFragmentInTangentSpace);

    // float height = 1- texture(displacementMap, texCoords).r;    
    // vec2 p = viewDir.xy / viewDir.z * (height * 0.4);
    // return texCoords.xy - p;   

    // number of depth layers
    const float minLayers = 8;
    const float maxLayers = 32;
    
    // TODO: make uniform
    const float heightScale = 0.4;

    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));  
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy * heightScale; 
    vec2 deltaTexCoords = P / numLayers;
  
    // get initial values
    vec2  currentTexCoords = texCoords.xy;
    float texZ = texCoords.z;
    float currentDepthMapValue = texture(displacementMap, vec3(currentTexCoords, texZ)).r;
      
    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(displacementMap, vec3(currentTexCoords, texZ)).r;  
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }
    
    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(displacementMap, vec3(prevTexCoords, texZ)).r - currentLayerDepth + layerDepth;
    
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}

vec3 CalculateEnvLightInfluence( float specularStrength, vec3 normal) {
    float diff = max(dot(normal, envLightDirection), 0.0);
    vec3 diffuse = diff * envLightDiffuse * envLightColor;

    vec3 viewDir = normalize(-cameraToFragmentPosition);
    // vec3 reflectDir = reflect(-lightDir, normal); // replace reflectDir with halfwayDir for blinn-phong lighting, which is better than phong lighting
    vec3 halfwayDir = normalize(envLightDirection + viewDir);
    float spec = pow(max(dot(viewDir, halfwayDir), 0.0), 32);
    vec3 specular = specularStrength * spec * envLightSpecular * envLightColor;  

    vec3 ambient = envLightColor * envLightAmbient;

    return ambient + diffuse + specular;
}

vec3 CalculateSpotlightInfluence(vec3 lightColor, vec3 rel_pos, float range, float innerAngle, float outerAngle, vec3 lightDirection, float specularStrength, vec3 normal) {
    float distance = length(rel_pos - cameraToFragmentPosition);
    vec3 lightDir = normalize(rel_pos - cameraToFragmentPosition); 
    
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 viewDir = normalize(-cameraToFragmentPosition);
    // vec3 reflectDir = reflect(-lightDir, normal); // replace reflectDir with halfwayDir for blinn-phong lighting, which is better than phong lighting
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(viewDir, halfwayDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    float theta = dot(lightDir, normalize(-lightDir));
    float spotlightStrength = range/pow(distance, 2) * (theta - outerAngle)/(innerAngle - outerAngle);
    
    return spotlightStrength * (diffuse + specular);
}

vec3 CalculateLightInfluence(vec3 lightColor, vec3 rel_pos, float range, float specularStrength, vec3 normal) {
    
    float distance = length(rel_pos - cameraToFragmentPosition);
    vec3 lightDir = normalize(rel_pos - cameraToFragmentPosition); 
    // float d = ;
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 viewDir = normalize(-cameraToFragmentPosition);
    // vec3 reflectDir = reflect(-lightDir, normal); // replace reflectDir with halfwayDir for blinn-phong lighting, which is better than phong lighting
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(viewDir, halfwayDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  

    vec3 ambient = lightColor * 0.1;

    float strength = range/pow(distance, 2);
    
    return strength * (ambient + diffuse + specular);
};

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
        realTexCoords = fragmentTexCoords;
    }
    
    vec3 normal = normalize(fragmentNormal);
    if (normalMappingEnabled) {normal = normalize(TBNmatrix * (texture(normalMap, realTexCoords).rgb * 2.0 - 1.0));} // todo: matrix multiplication in fragment shader is really bad, maybe?

    float specularStrength = 0.5;
    if (specularMappingEnabled) {
        specularStrength = texture(specularMap, realTexCoords).x;
    }

    vec3 light = vec3(0, 0, 0);
    for (uint i = 0; i < pointLightCount; i++) {
        light += CalculateLightInfluence(pointLights[i].colorAndRange.xyz, pointLights[i].rel_pos.xyz, pointLights[i].colorAndRange.w, specularStrength, normal);
    }
    for (uint i = 0; i < spotLightCount; i++) {
        light += CalculateSpotlightInfluence(spotLights[i].colorAndRange.xyz, spotLights[i].relPosAndInnerAngle.xyz, spotLights[i].colorAndRange.w, spotLights[i].relPosAndInnerAngle.w, spotLights[i].directionAndOuterAngle.w, spotLights[i].directionAndOuterAngle.xyz, specularStrength, normal);
    }
    light += CalculateEnvLightInfluence(specularStrength, normal);

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

    vec3 globalAmbient = vec3(0.1, 0.1, 0.1);
    vec4 color = tx * fragmentColor * vec4((light + globalAmbient), 1);
    Output = color;
    //Output = vec4(normalize(fragmentNormal), 1.0);
    //Output = vec4(-envLightDirection, 1.0);
    //Output = fragmentColor;
    //Output = vec4(dot(normal, envLightDirection));
};