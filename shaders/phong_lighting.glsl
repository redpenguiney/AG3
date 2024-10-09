uniform vec3 envLightDirection;
uniform vec3 envLightColor;
uniform float envLightDiffuse;
uniform float envLightAmbient;
uniform float envLightSpecular;

uniform uint pointLightCount;
uniform uint spotLightCount;
uniform uint pointLightOffset;
uniform uint spotLightOffset;

struct pointLight {
    vec4 colorAndRange; // w-coord is range, xyz is rgb
    vec4 rel_pos; // w-coord is padding
};

layout(std430, binding = 0) buffer pointLightSSBO {
    pointLight pointLights[];
};

struct spotLight {
    vec4 colorAndRange; // w-coord is range, xyz is rgb
    vec4 relPosAndInnerAngle; // w-coord is cos(inner angle)
    vec4 directionAndOuterAngle; // w-coord is cos(outer angle)
};

layout(std430, binding = 1) buffer spotLightSSBO {
    spotLight spotLights[];
};

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

    float theta = dot(lightDir, lightDirection);
    float spotlightStrength = range/pow(distance, 2) * max(0, (theta - outerAngle)/(innerAngle - outerAngle));
    
    //return vec3(theta, theta, theta);
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

vec3 CalculateLighting(float specularStrength, vec3 normal) {
    vec3 light = vec3(0, 0, 0);
    for (uint i = pointLightOffset; i < pointLightOffset + pointLightCount; i++) {
        light += CalculateLightInfluence(pointLights[i].colorAndRange.xyz, pointLights[i].rel_pos.xyz, pointLights[i].colorAndRange.w, specularStrength, normal);
    }
    for (uint i = spotLightOffset; i < spotLightOffset + spotLightCount; i++) {
        light += CalculateSpotlightInfluence(spotLights[i].colorAndRange.xyz, spotLights[i].relPosAndInnerAngle.xyz, spotLights[i].colorAndRange.w, spotLights[i].relPosAndInnerAngle.w, spotLights[i].directionAndOuterAngle.w, spotLights[i].directionAndOuterAngle.xyz, specularStrength, normal);
    }
    light += CalculateEnvLightInfluence(specularStrength, normal);


    return light;
}