#version 330 core

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;

uniform mat4 modelViewProjection;

out vec3 fColor;

void main()
{
  fColor = inColor;
  gl_Position = modelViewProjection * vec4(inPos, 1.0);
}
