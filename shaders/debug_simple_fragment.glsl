#version 330

in vec4 fragmentColor;

layout(location = 0) out vec4 Output;

void main() {
    Output = fragmentColor;
}
