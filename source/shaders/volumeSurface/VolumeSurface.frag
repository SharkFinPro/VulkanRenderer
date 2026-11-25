#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#include "../common/SceneLighting.glsl"
#include "../common/VolumePalette.glsl"

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) flat in uint fragMaterial;

layout(location = 0) out vec4 outColor;

void main()
{
  const vec3 albedo = volumePaletteAlbedo(fragMaterial);

  // Drawn double-sided, and caves are seen from inside
  vec3 normal = normalize(fragNormal);
  if (dot(normal, camera.position - fragPos) < 0.0)
  {
    normal = -normal;
  }

  vec3 result = vec3(0);
  for (int i = 0; i < numPointLights; i++)
  {
    float attenuation = lightAttenuation(pointLights[i].range, pointLights[i].position, fragPos);
    if (attenuation <= 0.0)
    {
      continue;
    }

    if (facesLight(normal, fragPos, pointLights[i].position) && pointLightShadow(i, fragPos) > 0.1)
    {
      result += StandardPointLightAffect(pointLights[i], albedo, normal, fragPos, camera.position, 32);
    }
    else
    {
      result += getStandardAmbient(pointLights[i].ambient, albedo) * attenuation;
    }
  }

  for (int i = 0; i < numSpotLights; i++)
  {
    float attenuation = lightAttenuation(spotLights[i].range, spotLights[i].position, fragPos);
    if (attenuation <= 0.0)
    {
      continue;
    }

    if (facesLight(normal, fragPos, spotLights[i].position) && spotLightShadow(i, fragPos) > 0.5)
    {
      result += StandardSpotLightAffect(spotLights[i], albedo, normal, fragPos, camera.position, 32);
    }
    else
    {
      result += getStandardAmbient(spotLights[i].ambient, albedo) * attenuation;
    }
  }

  outColor = vec4(result, 1.0);
}
