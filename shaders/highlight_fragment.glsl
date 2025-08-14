#version 330

layout(location = 0) out vec4 OutputColor;
layout(location = 1) out float OutputGeometry;

uniform vec2 screenSize;

void main() {
    OutputColor = vec4(gl_FragCoord.xy/screenSize, 0, 1);
    OutputGeometry = 1;
}
