#version 330

uniform float shaderTime;

layout(location = 0) out vec4 Output;


float rand(vec2 co){
    return fract((sin(dot(co.xy ,vec2(0.9898,78.233)))) * 4375.85453);
}

void main() {
	//Output = vec4(vec3(rand(gl_FragCoord.xy/200.0f + shaderTime/500025.0f) * 1.0f), 1.0);
	Output = vec4(vec3(rand(gl_FragCoord.xy/175.0f + shaderTime/600025.0f)),
		//rand(gl_FragCoord.xy/250.0f + shaderTime/500025.0f),
		//rand(gl_FragCoord.xy/200.0f + shaderTime/400025.0f),
	1.0);
}
