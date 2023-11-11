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

  // Vector V
  vec3 ViewDirection = vec3(0.0, 0.0, -1.0);
  vec3 V = -ViewDirection;

  // Vector R
  vec3 R = reflect(-L, N);

  // Specular term (R.V) ^ alpha
  float alpha = 10.0;
  float specular = pow(max(dot(R, V), 0.0), alpha);

  vec3 textureColor = texture(textureSampler, UV).rgb;
  vec3 finalColor = textureColor * lightIntensity * lambertian + specular;
  color = vec4(finalColor, 1.0);
}
