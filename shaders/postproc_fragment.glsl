#version 420 core
out vec4 FragColor;
  
in vec2 TexCoords;

layout(binding=0) uniform sampler2D screenTextureColor; // note: this syntax not ok until opengl 4.2
layout(binding=1) uniform sampler2D screenTextureAlpha; // note: this syntax not ok until opengl 4.2

const float offset = 1.0 / 1000.0;  

void main()
{ 
    vec2 offsets[9] = vec2[](
        vec2(-offset,  offset), // top-left
        vec2( 0.0f,    offset), // top-center
        vec2( offset,  offset), // top-right
        vec2(-offset,  0.0f),   // center-left
        vec2( 0.0f,    0.0f),   // center-center
        vec2( offset,  0.0f),   // center-right
        vec2(-offset, -offset), // bottom-left
        vec2( 0.0f,   -offset), // bottom-center
        vec2( offset, -offset)  // bottom-right    
    );

    float kernel[9] = float[](
        0.0675,  0.125, 0.0675,
        0.125,   0.25, 0.125,
        0.0675,  0.125, 0.0675
    );
    
    // do kernel-based post processing
    vec3 sampleTex[9];
    for(int i = 0; i < 9; i++)
    {
        // OIT finishes here
        vec4 accum = texture(screenTextureColor, TexCoords.st + offsets[i], 0);
        float reveal = texture(screenTextureAlpha, TexCoords.st + offsets[i], 0).r;
        sampleTex[i] = accum.rgb / max(accum.a, 1e-5);
        //sampleTex[i] = vec3(texture(screenTextureColor, TexCoords.st + offsets[i]));
    }
    vec3 col = vec3(0.0);
    for(int i = 0; i < 9; i++)
        col += sampleTex[i] * kernel[i];

    // tonemapping
    vec3 mapped = col; //col / (col + vec3(1.0));
    FragColor = vec4(mapped, 1.0);
    //FragColor = vec4(texture(screenTextureColor, vec2(TexCoords.x, 1-TexCoords.y)).xxx, 1.0);
    //FragColor = vec4(TexCoords, 0.5, 1.0);
    //FragColor = vec4(0, 1, 1, 1);
    //float depthValue = texture(screenTextureColor, TexCoords).r;
    //FragColor = vec4(vec3(depthValue), 1.0);
    FragColor = vec4(texture(screenTextureColor, (TexCoords.st), 0).xyz, 1.0);
    //FragColor = vec4(texture(screenTextureAlpha, (TexCoords.st), 0).rrr, 1.0);
    //FragColor = vec4(1.0, 1.0, 0.0, 1.0);   
}