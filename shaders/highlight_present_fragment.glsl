#version 420 core
out vec4 FragColor;
  
in vec2 TexCoords;

layout(binding=0) uniform sampler2D outline; // note: this syntax not ok until opengl 4.2

void main() {
	FragColor = texture(outline, TexCoords);
}