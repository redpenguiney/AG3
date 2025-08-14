#version 420 core
out vec4 FragColor;
  
in vec2 TexCoords;

layout(binding=0) uniform sampler2D outline; // note: this syntax not ok until opengl 4.2

void main() {
	vec4 result = texture(outline, TexCoords);
	if (result.xy == vec2(0, 0))
		discard;
	FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}