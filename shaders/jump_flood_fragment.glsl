#version 430

in vec2 TexCoords;

layout(location = 0) out vec4 Output;

layout(binding=0) uniform sampler2D geometryMask; // note: this syntax not ok until opengl 4.2

uniform vec2 screenSize;
uniform float stepSize;


vec3 jump(vec3 minSeed, vec2 current, vec2 offset) {
    vec2 samplePos = current + offset;
    if (length(clamp(samplePos,0,1) - samplePos) > .0001f) { // out of bounds check
        return minSeed;
    } 
    vec2 seed = texture2D(geometryMask, samplePos).rg;
    vec2 cScaled = floor(current * textureSize(geometryMask,0));
    vec2 sScaled = floor(seed * textureSize(geometryMask,0));
    float dist = distance(cScaled, sScaled);
    if (dist < minSeed.z) {
        return vec3(seed.x, seed.y, dist);
    }
    return minSeed;
}


void main() {
    vec2 jumpDist = round(stepSize) / screenSize;

    vec3 curr = vec3(1,1,9999999);
    curr = jump(curr, TexCoords, jumpDist * vec2( 0,  0)); // cc
    curr = jump(curr, TexCoords, jumpDist * vec2( 0, +1)); // nn
    curr = jump(curr, TexCoords, jumpDist * vec2(+1, +1)); // ne
    curr = jump(curr, TexCoords, jumpDist * vec2(+1,  0)); // ee
    curr = jump(curr, TexCoords, jumpDist * vec2(+1, -1)); // se
    curr = jump(curr, TexCoords, jumpDist * vec2( 0, -1)); // ss
    curr = jump(curr, TexCoords, jumpDist * vec2(-1, -1)); // sw
    curr = jump(curr, TexCoords, jumpDist * vec2(-1,  0)); // ww
    curr = jump(curr, TexCoords, jumpDist * vec2(-1, +1)); // nw

    Output = vec4(curr.xy, 0, 1);
    
}
