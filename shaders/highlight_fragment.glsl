#version 330

layout(location = 0) out vec4 OutputColor;
layout(location = 1) out float OutputGeometry

void main() {
    OutputColor = vec4(gl_FragCoord.xyz, 1);
    OutputGeometry = 1;
}
