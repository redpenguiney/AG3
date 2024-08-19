#version 330 core
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2DArray screenTexture;

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
        sampleTex[i] = vec3(texture(screenTexture, vec3(TexCoords.st + offsets[i], 0.0)));
    }
    vec3 col = vec3(0.0);
    for(int i = 0; i < 9; i++)
        col += sampleTex[i] * kernel[i];

    // tonemapping
    vec3 mapped = col / (col + vec3(1.0));
    FragColor = vec4(mapped, 1.0);
    // FragColor = vec4(texture(screenTexture, vec2(TexCoords.x, 1-TexCoords.y)).xxx, 1.0);
    //FragColor = vec4(TexCoords, 0.5, 1.0);
    //FragColor = vec4(1, 1, 1, 0.5);
    //float depthValue = texture(screenTexture, TexCoords).r;
    // FragColor = vec4(vec3(depthValue), 1.0);
}