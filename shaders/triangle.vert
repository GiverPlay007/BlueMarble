#version 330 core

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;

uniform mat4 modelViewProjection;

out vec3 fColor;
out vec2 UV;

void main()
{
  fColor = inColor;
  UV = inUV;
  gl_Position = modelViewProjection * vec4(inPos, 1.0);
}
