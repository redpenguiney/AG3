#version 330

layout(location = 0) out vec4 Output;

in vec3 fragmentTexCoords;

uniform samplerCube skybox;

void main() {
    gl_FragData[0] = vec4(texture(skybox, fragmentTexCoords));
    // Output = vec4(fragmentTexCoords, 1.0);
    //Output = vec4(gl_FragCoord.zzz, 1.0);
}