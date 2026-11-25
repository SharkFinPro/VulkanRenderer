struct PointLight {
  mat4[6] lightViewProjections;
  vec3 position;
  float shadowNearPlane; // Near plane of the shadow projections
  vec3 color;
  int shadowMapIndex; // Into pointLightShadowMaps, or -1 when the light has none
  float ambient;
  float diffuse;
  float specular;
  float shadowRange; // Distance the shadow map covers; lit beyond it
  float range; // 0 when the light has no range
};

struct SpotLight {
  mat4 lightViewProjection;
  vec3 position;
  float ambient;
  vec3 color;
  float diffuse;
  vec3 direction;
  float specular;
  float coneAngle;
  float shadowRange; // Distance the shadow map covers; lit beyond it
  int shadowMapIndex; // Into spotLightShadowMaps, or -1 when the light has none
  float range; // 0 when the light has no range
};

struct DirectionalLight {
  mat4 lightViewProjection;
  vec3 direction; // The direction the light travels, normalized
  float ambient;
  vec3 color;
  float diffuse;
  float specular;
  int shadowMapIndex; // Into directionalLightShadowMaps, or -1 when the light has none
  float shadowBias; // In normalized depth
  float pad0;
};

// 1 for a light without a range (0). Otherwise inverse square in world units, windowed smoothly to 0 at the range;
// LightRange.h mirrors it on the CPU.
float lightAttenuation(float range, vec3 lightPosition, vec3 fragPos)
{
  if (range <= 0.0)
  {
    return 1.0;
  }

  vec3 toLight = lightPosition - fragPos;
  float distSq = dot(toLight, toLight);
  float rangeSq = range * range;

  if (distSq >= rangeSq)
  {
    return 0.0;
  }

  float ratio = distSq / rangeSq;
  float window = clamp(1.0 - ratio * ratio, 0.0, 1.0);

  return window * window / (distSq + 1.0);
}

bool isInSpotlight(SpotLight light, vec3 fragPos)
{
  float cutoffAngle = cos(light.coneAngle);
  vec3 lightToFrag = normalize(fragPos - light.position);

  float theta = dot(lightToFrag, normalize(light.direction));

  return theta >= cutoffAngle;
}

// False where the surface faces away from the light, which then adds no diffuse or specular light whether or not
// anything is in the way, so callers skip the shadow test and take the shadowed result
bool facesLight(vec3 normal, vec3 fragPos, vec3 lightPosition)
{
  return dot(normal, lightPosition - fragPos) > 0.0;
}

// The same for a directional light, whose light vector is the reverse of its direction
bool facesDirectionalLight(vec3 normal, vec3 lightDirection)
{
  return dot(normal, -lightDirection) > 0.0;
}

vec3 getStandardAmbient(float lightAmbient, vec3 color)
{
  vec3 ambient = lightAmbient * color;

  return ambient;
}

// lightDir points from the fragment toward the light, normalized
vec3 getDiffuseFromDirection(vec3 lightDir, float lightDiffuse, vec3 normal, vec3 color)
{
  float d = max(dot(normal, lightDir), 0.0);
  vec3 diffuse = lightDiffuse * d * color;

  return diffuse;
}

vec3 getSpecularFromDirection(vec3 lightDir, float lightSpecular, vec3 lightColor, vec3 cameraPosition, vec3 fragPos, vec3 normal, float shininess)
{
  vec3 specular = vec3(0);

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

vec3 getStandardDiffuse(vec3 lightPosition, float lightDiffuse, vec3 fragPos, vec3 normal, vec3 color)
{
  return getDiffuseFromDirection(normalize(lightPosition - fragPos), lightDiffuse, normal, color);
}

vec3 getStandardSpecular(vec3 lightPosition, float lightSpecular, vec3 lightColor, vec3 cameraPosition, vec3 fragPos, vec3 normal, float shininess)
{
  return getSpecularFromDirection(normalize(lightPosition - fragPos), lightSpecular, lightColor, cameraPosition, fragPos, normal, shininess);
}

vec3 StandardPointLightAffect(PointLight light,
                              vec3 color,
                              vec3 normal,
                              vec3 fragPos,
                              vec3 cameraPosition,
                              float shininess)
{
  float attenuation = lightAttenuation(light.range, light.position, fragPos);

  if (attenuation <= 0.0)
  {
    return vec3(0);
  }

  vec3 normalizedNormal = normalize(normal);

  vec3 ambient = getStandardAmbient(light.ambient, color);
  vec3 diffuse = getStandardDiffuse(light.position, light.diffuse, fragPos, normalizedNormal, color);
  vec3 specular = getStandardSpecular(light.position, light.specular, light.color, cameraPosition, fragPos, normalizedNormal, shininess);

  return (ambient + diffuse + specular) * light.color * attenuation;
}

vec3 StandardSpotLightAffect(SpotLight light,
                              vec3 color,
                              vec3 normal,
                              vec3 fragPos,
                              vec3 cameraPosition,
                              float shininess)
{
  float attenuation = lightAttenuation(light.range, light.position, fragPos);

  if (attenuation <= 0.0)
  {
    return vec3(0);
  }

  if (!isInSpotlight(light, fragPos))
  {
    vec3 ambient = getStandardAmbient(light.ambient, color);

    return ambient * light.color * attenuation;
  }

  vec3 normalizedNormal = normalize(normal);

  vec3 ambient = getStandardAmbient(light.ambient, color);
  vec3 diffuse = getStandardDiffuse(light.position, light.diffuse, fragPos, normalizedNormal, color);
  vec3 specular = getStandardSpecular(light.position, light.specular, light.color, cameraPosition, fragPos, normalizedNormal, shininess);

  return (ambient + diffuse + specular) * light.color * attenuation;
}

vec3 SpecularMapPointLightAffect(PointLight light,
                                 vec3 color,
                                 vec3 specColor,
                                 vec3 normal,
                                 vec3 fragPos,
                                 vec3 cameraPosition,
                                 float shininess)
{
  float attenuation = lightAttenuation(light.range, light.position, fragPos);

  if (attenuation <= 0.0)
  {
    return vec3(0);
  }

  vec3 normalizedNormal = normalize(normal);

  vec3 ambient = getStandardAmbient(light.ambient, color);
  vec3 diffuse = getStandardDiffuse(light.position, light.diffuse, fragPos, normalizedNormal, color);
  vec3 specular = getStandardSpecular(light.position, light.specular, light.color, cameraPosition, fragPos, normalizedNormal, shininess) * specColor;

  return (ambient + diffuse + specular) * light.color * attenuation;
}

vec3 SpecularMapSpotLightAffect(SpotLight light,
                                 vec3 color,
                                 vec3 specColor,
                                 vec3 normal,
                                 vec3 fragPos,
                                 vec3 cameraPosition,
                                 float shininess)
{
  float attenuation = lightAttenuation(light.range, light.position, fragPos);

  if (attenuation <= 0.0)
  {
    return vec3(0);
  }

  if (!isInSpotlight(light, fragPos))
  {
    vec3 ambient = getStandardAmbient(light.ambient, color);

    return ambient * light.color * attenuation;
  }

  vec3 normalizedNormal = normalize(normal);

  vec3 ambient = getStandardAmbient(light.ambient, color);
  vec3 diffuse = getStandardDiffuse(light.position, light.diffuse, fragPos, normalizedNormal, color);
  vec3 specular = getStandardSpecular(light.position, light.specular, light.color, cameraPosition, fragPos, normalizedNormal, shininess) * specColor;

  return (ambient + diffuse + specular) * light.color * attenuation;
}

vec3 SmokePointLightAffect(PointLight light, vec3 color, vec3 fragPos)
{
  // Calculate distance
  vec3 lightToFrag = light.position - fragPos;
  float dist = length(lightToFrag);

  // Calculate attenuation
  float attenuation = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);

  // Combined Output
  return (light.ambient + light.diffuse) * color * light.color * attenuation; // Color * Color for brighter color
}

vec3 SmokeSpotLightAffect(SpotLight light, vec3 color, vec3 fragPos)
{
  // Calculate distance
  vec3 lightToFrag = light.position - fragPos;
  float dist = length(lightToFrag);

  // Calculate attenuation
  float attenuation = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);

  if (!isInSpotlight(light, fragPos))
  {
    // Combined Output
    return (light.ambient) * color * light.color * attenuation; // Color * Color for brighter color
  }

  // Combined Output
  return (light.ambient + light.diffuse) * color * light.color * attenuation; // Color * Color for brighter color
}

vec3 StandardDirectionalLightAffect(DirectionalLight light,
                                    vec3 color,
                                    vec3 normal,
                                    vec3 fragPos,
                                    vec3 cameraPosition,
                                    float shininess)
{
  vec3 lightDir = -light.direction;
  vec3 normalizedNormal = normalize(normal);

  vec3 ambient = getStandardAmbient(light.ambient, color);
  vec3 diffuse = getDiffuseFromDirection(lightDir, light.diffuse, normalizedNormal, color);
  vec3 specular = getSpecularFromDirection(lightDir, light.specular, light.color, cameraPosition, fragPos, normalizedNormal, shininess);

  return (ambient + diffuse + specular) * light.color;
}

vec3 SpecularMapDirectionalLightAffect(DirectionalLight light,
                                       vec3 color,
                                       vec3 specColor,
                                       vec3 normal,
                                       vec3 fragPos,
                                       vec3 cameraPosition,
                                       float shininess)
{
  vec3 lightDir = -light.direction;
  vec3 normalizedNormal = normalize(normal);

  vec3 ambient = getStandardAmbient(light.ambient, color);
  vec3 diffuse = getDiffuseFromDirection(lightDir, light.diffuse, normalizedNormal, color);
  vec3 specular = getSpecularFromDirection(lightDir, light.specular, light.color, cameraPosition, fragPos, normalizedNormal, shininess) * specColor;

  return (ambient + diffuse + specular) * light.color;
}

vec3 SmokeDirectionalLightAffect(DirectionalLight light, vec3 color)
{
  return (light.ambient + light.diffuse) * color * light.color; // Color * Color for brighter color
}
