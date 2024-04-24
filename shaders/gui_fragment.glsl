#version 430

in vec4 fragmentColor;
in vec3 fragmentTexCoords;
// in vec4 lightSpaceCoords;

layout(location = 0) out vec4 Output;

layout(binding=4) uniform sampler2DArray fontMap; // note: this syntax not ok until opengl 4.2
layout(binding=0) uniform sampler2DArray colorMap; // note: this syntax not ok until opengl 4.2

uniform bool fontMappingEnabled;
uniform bool colorMappingEnabled;

// TODO; to avoid color banding add dithering 
void main()
{   
    vec4 colorTx = vec4(1.0, 1.0, 1.0, 1.0);
    float font = 1.0;
    if (fragmentTexCoords.z >= 0 && colorMappingEnabled) {
        colorTx = texture(colorMap, fragmentTexCoords);
    }
    if (fragmentTexCoords.z >= 0 && fontMappingEnabled) {
        font = texture(fontMap, fragmentTexCoords).r;
    }

    

    vec4 color = fragmentColor * colorTx;
    color.a *= font;

    if (color.a < 0.1) {
        discard;
    };

    Output = color;
};