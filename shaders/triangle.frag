#version 330 core

uniform sampler2D textureSampler;
uniform vec3 lightDirection;
uniform float lightIntensity;

in vec3 normal;
in vec3 fColor;
in vec2 UV;

out vec4 color;

void main()
{
  // Phong shading
  vec3 N = normalize(normal);
  vec3 L = -normalize(lightDirection);

  float lambertian = max(dot(N, L), 0.0);

  vec3 textureColor = texture(textureSampler, UV).rgb;
  vec3 finalColor = textureColor * lightIntensity * lambertian;
  color = vec4(finalColor, 1.0);
}
