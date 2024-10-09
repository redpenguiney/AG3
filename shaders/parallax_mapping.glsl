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

