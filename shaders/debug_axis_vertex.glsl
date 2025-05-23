#version 330

layout(location=0) in vec3 vertexPos;
layout(location=1) in vec4 vertexColor;
layout(location=2) in mat4 modelMatrix;
// locations 2-5 are part of model

out vec4 fragmentColor;

uniform float stretch;

void main()
{
    gl_Position = vec4(stretch, 1.0, 1.0, 1.0) * (modelMatrix * vec4(vertexPos, 1.0));
    fragmentColor = vertexColor;
}
    

