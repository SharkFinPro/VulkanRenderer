#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#include "../common/SceneLighting.glsl"

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) flat in uint fragMaterial;

layout(location = 0) out vec4 outColor;

// A flat albedo by material until surfaces get textured materials
const vec3 palette[8] = vec3[](
  vec3(0.60, 0.60, 0.60),
  vec3(0.55, 0.75, 0.40),
  vec3(0.70, 0.55, 0.35),
  vec3(0.55, 0.60, 0.80),
  vec3(0.85, 0.80, 0.55),
  vec3(0.80, 0.50, 0.50),
  vec3(0.50, 0.75, 0.75),
  vec3(0.75, 0.60, 0.80)
);

void main()
{
  const vec3 albedo = palette[fragMaterial % 8];

  // Drawn double-sided, and caves are seen from inside
  vec3 normal = normalize(fragNormal);
  if (dot(normal, camera.position - fragPos) < 0.0)
  {
    normal = -normal;
  }

  vec3 result = vec3(0);
  for (int i = 0; i < numPointLights; i++)
  {
    if (pointLightShadow(i, fragPos) > 0.1)
    {
      result += StandardPointLightAffect(pointLights[i], albedo, normal, fragPos, camera.position, 32);
    }
    else
    {
      result += getStandardAmbient(pointLights[i].ambient, albedo);
    }
  }

  for (int i = 0; i < numSpotLights; i++)
  {
    if (spotLightShadow(i, fragPos) > 0.5)
    {
      result += StandardSpotLightAffect(spotLights[i], albedo, normal, fragPos, camera.position, 32);
    }
    else
    {
      result += getStandardAmbient(spotLights[i].ambient, albedo);
    }
  }

  outColor = vec4(result, 1.0);
}
