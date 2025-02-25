#version 450

struct PointLight {
  vec3 position;
  float padding1; // Padding to ensure alignment
  vec3 color;
  float padding2; // Padding to ensure alignment
  float ambient;
  float diffuse;
  float specular;
  float padding3; // Padding to ensure alignment
};

layout(set = 0, binding = 2) uniform PointLightsMetadata {
  int numLights;
};

layout(set = 0, binding = 5) readonly buffer PointLights {
  PointLight lights[];
};

layout(set = 0, binding = 3) uniform Camera {
  vec3 position;
} camera;

layout(set = 0, binding = 4) uniform Snake {
  float wiggle;
} snake;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

vec3 PointLightAffect(PointLight light, vec3 color)
{
  // Ambient
  vec3 ambient = light.ambient * color;

  // Diffuse
  vec3 norm = normalize(fragNormal);
  vec3 lightDir = normalize(light.position - fragPos);
  float d = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = light.diffuse * d * color;

  // Specular
  vec3 specular = vec3(0);
  if(d > 0.0) // only do specular if the light can see the point
  {
    vec3 viewDir = normalize(camera.position - fragPos);
    vec3 reflectDir = normalize(reflect(-lightDir, norm));
    float cosphi = dot(viewDir, reflectDir);

    if (cosphi > 0.0)
    {
      specular = pow(cosphi, 10) * light.specular * light.color; // 10 = shininess
    }
  }

  // Combined Output
  return (ambient + diffuse + specular) * light.color;
}

vec3 hsvToRgb(float h, float s, float v)
{
  float c = v * s;
  float x = c * (1.0 - abs(mod(h * 6.0, 2.0) - 1.0));
  float m = v - c;

  vec3 rgb;

  if (h < 1.0 / 6.0)      rgb = vec3(c, x, 0.0);
  else if (h < 2.0 / 6.0) rgb = vec3(x, c, 0.0);
  else if (h < 3.0 / 6.0) rgb = vec3(0.0, c, x);
  else if (h < 4.0 / 6.0) rgb = vec3(0.0, x, c);
  else if (h < 5.0 / 6.0) rgb = vec3(x, 0.0, c);
  else                    rgb = vec3(c, 0.0, x);

  return rgb + m;
}

void main()
{
  float pos = (fragPos.x + 11.2);
  float p = pos / 18.0;

  vec3 color = hsvToRgb(mix(1.0 / 6.0, 5.0 / 6.0, p), 1, 1);

  float tension = abs(sin(fragPos.x * 0.5) * snake.wiggle);
  color.g -= tension;
  color.b -= tension;
  color.r += tension;

  vec3 result = vec3(0);
  for (int i = 0; i < numLights; i++)
  {
    result += PointLightAffect(lights[i], color);
  }

  outColor = vec4(result, 1.0);
}