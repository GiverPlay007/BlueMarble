#version 330 core

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec2 inUV;

uniform mat4 normalMatrix;
uniform mat4 modelViewProjection;

out vec3 normal;
out vec3 fColor;
out vec2 UV;

void main()
{
  normal = vec3(normalMatrix * vec4(inNormal, 0.0));
  fColor = inColor;
  UV = inUV;
  gl_Position = modelViewProjection * vec4(inPos, 1.0);
}
