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

vec3 getStandardAmbient(PointLight light, vec3 color)
{
  vec3 ambient = light.ambient * color;

  return ambient;
}

vec3 getStandardDiffuse(PointLight light, vec3 fragPos, vec3 normal, vec3 color)
{
  vec3 lightDir = normalize(light.position - fragPos);
  float d = max(dot(normal, lightDir), 0.0);
  vec3 diffuse = light.diffuse * d * color;

  return diffuse;
}

vec3 getStandardSpecular(PointLight light, vec3 cameraPosition, vec3 fragPos, vec3 normal, float shininess)
{
  vec3 specular = vec3(0);

  vec3 lightDir = normalize(light.position - fragPos);
  float d = max(dot(normal, lightDir), 0.0);
  if(d > 0.0) // only do specular if the light can see the point
  {
    vec3 viewDir = normalize(cameraPosition - fragPos);
    vec3 reflectDir = normalize(reflect(-lightDir, normal));
    float cosphi = dot(viewDir, reflectDir);

    if (cosphi > 0.0)
    {
      specular = pow(cosphi, shininess) * light.specular * light.color;
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

  vec3 ambient = getStandardAmbient(light, color);
  vec3 diffuse = getStandardDiffuse(light, fragPos, normalizedNormal, color);
  vec3 specular = getStandardSpecular(light, cameraPosition, fragPos, normalizedNormal, shininess);

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

  vec3 ambient = getStandardAmbient(light, color);
  vec3 diffuse = getStandardDiffuse(light, fragPos, normalizedNormal, color);
  vec3 specular = getStandardSpecular(light, cameraPosition, fragPos, normalizedNormal, shininess) * specColor;

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