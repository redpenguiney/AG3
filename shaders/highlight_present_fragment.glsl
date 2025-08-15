#version 420 core
out vec4 FragColor;
  
in vec2 TexCoords;

layout(binding=0) uniform sampler2D outline; // note: this syntax not ok until opengl 4.2

void main() {
	vec4 result = texture(outline, TexCoords);
	float dist = result.z;
	if (dist <= 0.0f || dist > 2.0f)
		discard;
	FragColor = vec4(1, 1, 1, 1);
}