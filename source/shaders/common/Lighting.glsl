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

struct SpotLight {
  vec3 position;
  float ambient;
  vec3 color;
  float diffuse;
  vec3 direction;
  float specular;
  float coneAngle;
};

vec3 getStandardAmbient(float lightAmbient, vec3 color)
{
  vec3 ambient = lightAmbient * color;

  return ambient;
}

vec3 getStandardDiffuse(vec3 lightPosition, float lightDiffuse, vec3 fragPos, vec3 normal, vec3 color)
{
  vec3 lightDir = normalize(lightPosition - fragPos);
  float d = max(dot(normal, lightDir), 0.0);
  vec3 diffuse = lightDiffuse * d * color;

  return diffuse;
}

vec3 getStandardSpecular(vec3 lightPosition, float lightSpecular, vec3 lightColor, vec3 cameraPosition, vec3 fragPos, vec3 normal, float shininess)
{
  vec3 specular = vec3(0);

  vec3 lightDir = normalize(lightPosition - fragPos);
  float d = max(dot(normal, lightDir), 0.0);
  if(d > 0.0) // only do specular if the light can see the point
  {
    vec3 viewDir = normalize(cameraPosition - fragPos);
    vec3 reflectDir = normalize(reflect(-lightDir, normal));
    float cosphi = dot(viewDir, reflectDir);

    if (cosphi > 0.0)
    {
      specular = pow(cosphi, shininess) * lightSpecular * lightColor;
    }
  }

  return specular;
}

vec3 StandardPointLightAffect(PointLight light,
                              vec3 color,
                              vec3 normal,
                              vec3 fragPos,
                              vec3 cameraPosition,
                              float shininess)
{
  vec3 normalizedNormal = normalize(normal);

  vec3 ambient = getStandardAmbient(light.ambient, color);
  vec3 diffuse = getStandardDiffuse(light.position, light.diffuse, fragPos, normalizedNormal, color);
  vec3 specular = getStandardSpecular(light.position, light.specular, light.color, cameraPosition, fragPos, normalizedNormal, shininess);

  return (ambient + diffuse + specular) * light.color;
}

vec3 StandardSpotLightAffect(SpotLight light,
                              vec3 color,
                              vec3 normal,
                              vec3 fragPos,
                              vec3 cameraPosition,
                              float shininess)
{
  float cutoffAngle = cos(light.coneAngle);
  vec3 lightToFrag = normalize(fragPos - light.position);

  float theta = dot(lightToFrag, normalize(light.direction));
  if (theta < cutoffAngle)
  {
    vec3 ambient = getStandardAmbient(light.ambient, color);

    return ambient * light.color;
  }

  vec3 normalizedNormal = normalize(normal);

  vec3 ambient = getStandardAmbient(light.ambient, color);
  vec3 diffuse = getStandardDiffuse(light.position, light.diffuse, fragPos, normalizedNormal, color);
  vec3 specular = getStandardSpecular(light.position, light.specular, light.color, cameraPosition, fragPos, normalizedNormal, shininess);

  return (ambient + diffuse + specular) * light.color;
}

vec3 SpecularMapPointLightAffect(PointLight light,
                                 vec3 color,
                                 vec3 specColor,
                                 vec3 normal,
                                 vec3 fragPos,
                                 vec3 cameraPosition,
                                 float shininess)
{
  vec3 normalizedNormal = normalize(normal);

  vec3 ambient = getStandardAmbient(light.ambient, color);
  vec3 diffuse = getStandardDiffuse(light.position, light.diffuse, fragPos, normalizedNormal, color);
  vec3 specular = getStandardSpecular(light.position, light.specular, light.color, cameraPosition, fragPos, normalizedNormal, shininess) * specColor;

  return (ambient + diffuse + specular) * light.color;
}

vec3 SpecularMapSpotLightAffect(SpotLight light,
                                 vec3 color,
                                 vec3 specColor,
                                 vec3 normal,
                                 vec3 fragPos,
                                 vec3 cameraPosition,
                                 float shininess)
{
  float cutoffAngle = cos(light.coneAngle);
  vec3 lightToFrag = normalize(fragPos - light.position);

  float theta = dot(lightToFrag, normalize(light.direction));
  if (theta < cutoffAngle)
  {
    vec3 ambient = getStandardAmbient(light.ambient, color);

    return ambient * light.color;
  }

  vec3 normalizedNormal = normalize(normal);

  vec3 ambient = getStandardAmbient(light.ambient, color);
  vec3 diffuse = getStandardDiffuse(light.position, light.diffuse, fragPos, normalizedNormal, color);
  vec3 specular = getStandardSpecular(light.position, light.specular, light.color, cameraPosition, fragPos, normalizedNormal, shininess) * specColor;

  return (ambient + diffuse + specular) * light.color;
}

vec3 SmokePointLightAffect(PointLight light, vec3 color, vec3 fragPos)
{
  // Calculate distance
  vec3 lightToFrag = light.position - fragPos;
  float dist = length(lightToFrag);

  // Calculate attenuation
  float attenuation = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);

  // Combined Output
  return (light.ambient + light.diffuse) * light.color * light.color * attenuation; // Color * Color for brighter color
}