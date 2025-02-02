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

layout(set = 0, binding = 4) uniform Curtain {
  float amplitude;
  float period;
  float shininess;
} curtain;

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
      specular = pow(cosphi, curtain.shininess) * light.specular * light.color;
    }
  }

  // Combined Output
  return (ambient + diffuse + specular) * light.color;
}

void main()
{
  vec3 fragColor = vec3(1, 0, 1);

  // now use fragColor in the per-fragment lighting equations:
  vec3 result = vec3(0);
  for (int i = 0; i < numLights; i++)
  {
    result += PointLightAffect(lights[i], fragColor);
  }

  outColor = vec4(result, 1.0);
}