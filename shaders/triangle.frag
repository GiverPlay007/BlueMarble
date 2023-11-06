#version 330 core

uniform sampler2D textureSampler;

in vec3 fColor;
in vec2 UV;

out vec4 color;

void main()
{
  vec3 textureColor = texture(textureSampler, UV).rgb;
  color = vec4(textureColor, 1.0);
}
