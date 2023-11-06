#version 330 core

in vec3 fColor;
in vec2 UV;

out vec4 color;

void main()
{
  // color = vec4(fColor, 1.0);
  color = vec4(UV, 0.0, 1.0);
}
