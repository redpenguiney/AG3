#version 330

layout(location = 0) out vec4 Output;

in vec3 fragmentTexCoords;

uniform samplerCube skybox;

void main() {
    Output = texture(skybox, fragmentTexCoords);
    //Output = vec4(fragmentTexCoords, 1.0);
}