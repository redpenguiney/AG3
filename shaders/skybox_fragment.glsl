#version 330

layout(location = 0) out vec4 Output;

in vec3 fragmentTexCoords;

uniform samplerCube skybox;

void main() {
    Output = vec4(0.5, 0.5, 0.5, 1.0) + texture(skybox, fragmentTexCoords);
}