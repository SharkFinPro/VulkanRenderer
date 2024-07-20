#version 450

layout(set = 1, binding = 1) uniform sampler2D texSampler;
layout(set = 1, binding = 4) uniform sampler2D specSampler;

layout(set = 0, binding = 2) uniform Light {
  vec3 position;
  vec3 color;

  float ambient;
  float diffuse;
  float specular;
} light;

layout(set = 0, binding = 3) uniform Camera {
  vec3 position;
} camera;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main()
{
  vec3 texColor = texture(texSampler, fragTexCoord).rgb;
  vec3 specColor = texture(specSampler, fragTexCoord).rgb;

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
  vec3 result = (ambient + diffuse + specular) * light.color;
  outColor = vec4(result, 1.0);
}