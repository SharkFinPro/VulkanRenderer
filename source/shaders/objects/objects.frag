#version 450
#define MAX_LIGHTS 10

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

layout(set = 1, binding = 1) uniform sampler2D texSampler;
layout(set = 1, binding = 4) uniform sampler2D specSampler;

layout(set = 0, binding = 2) uniform PointLightsMetadata {
  int numLights;
};

layout(set = 0, binding = 5) uniform PointLights {
  PointLight lights[MAX_LIGHTS];
};

layout(set = 0, binding = 3) uniform Camera {
  vec3 position;
} camera;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

vec3 PointLightAffect(PointLight light, vec3 texColor, vec3 specColor)
{
  // ambient
  vec3 ambient = light.ambient * texColor;

  // diffuse
  vec3 norm = normalize(fragNormal);
  vec3 lightDir = normalize(light.position - fragPos);
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = light.diffuse * diff * texColor;

  // specular
  vec3 viewDir = normalize(camera.position - fragPos);
  vec3 reflectDir = reflect(-lightDir, norm);
  //  float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
  vec3 specular = light.specular * spec * specColor;

  //
  return (ambient + diffuse + specular) * light.color;
}

void main()
{
  vec3 texColor = texture(texSampler, fragTexCoord).rgb;
  vec3 specColor = texture(specSampler, fragTexCoord).rgb;

  vec3 result = vec3(0);
  for (int i = 0; i < numLights; i++)
  {
    result += PointLightAffect(lights[i], texColor, specColor);
  }

  outColor = vec4(result, 1.0);
}