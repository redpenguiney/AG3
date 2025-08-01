#version 430

layout(location = 0) out vec4 Output;

layout(binding=0) uniform sampler2D geometryMask; // note: this syntax not ok until opengl 4.2



void main() {
    OutputColor = vec4(gl_FragCoord.xyz, 1);
    OutputGeometry = 1;
}
