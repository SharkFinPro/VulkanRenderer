#version 450

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 4) uniform sampler2D specSampler;

layout(binding = 2) uniform Light {
  vec3 position;
  vec3 color;

  float ambient;
  float diffuse;
  float specular;
} light;

layout(binding = 3) uniform Camera {
  vec3 position;
} camera;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main()
{
  // ambient
  vec3 ambient = light.ambient * texture(texSampler, fragTexCoord).rgb;

  // diffuse
  vec3 norm = normalize(fragNormal);
  vec3 lightDir = normalize(light.position - fragColor);
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = light.diffuse * diff * texture(texSampler, fragTexCoord).rgb;

  // specular
  vec3 viewDir = normalize(camera.position - fragColor);
  vec3 reflectDir = reflect(-lightDir, norm);
//  float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
  vec3 specular = light.specular * spec * texture(specSampler, fragTexCoord).rgb;

  //
  vec3 result = (ambient + diffuse + specular) * light.color;
  outColor = vec4(result, 1.0);

//  outColor = vec4(fragColor * texture(texSampler, fragTexCoord).rgb, 1.0);
}