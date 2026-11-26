#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_texture_shadow_lod : require
#extension GL_EXT_control_flow_attributes : require
#include "../common/SceneLighting.glsl"
#include "../common/VolumePalette.glsl"

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) flat in uint fragMaterial;

layout(location = 0) out vec4 outColor;

void main()
{
  const vec3 albedo = volumePaletteAlbedo(fragMaterial);

  // Drawn double-sided, so a back face (the surface seen from inside the solid) is lit as its other side. Which side is
  // in view is the rasterizer's to say: the meshes wind consistently, outward faces front. Judging it from the shading
  // normal instead misfires where that normal leans away from the camera on a face that is in front of it (a thin lip,
  // whose interpolated normals bend around it): the normal was flipped to face away from every light, which left a line
  // of ambient-only, nearly black pixels along the lip.
  vec3 normal = normalize(fragNormal);
  if (!gl_FrontFacing)
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

    float shadow = facesLight(normal, fragPos, pointLights[i].position) ? pointLightShadow(i, fragPos, normal) : 0.0;
    vec3 shadowed = getStandardAmbient(pointLights[i].ambient, albedo) * attenuation;

    // The shadow is a fraction, so the edge of a shadow blends from one result to the other
    result += shadow > 0.0
      ? mix(shadowed, StandardPointLightAffect(pointLights[i], albedo, normal, fragPos, camera.position, 32), shadow)
      : shadowed;
  }

  for (int i = 0; i < numSpotLights; i++)
  {
    float attenuation = lightAttenuation(spotLights[i].range, spotLights[i].position, fragPos);
    if (attenuation <= 0.0)
    {
      continue;
    }

    // Outside the cone the light adds only its ambient whatever the shadow, so skip the lookup there
    float shadow = !facesLight(normal, fragPos, spotLights[i].position) ? 0.0
                 : !isInSpotlight(spotLights[i], fragPos)               ? 1.0
                                                                         : spotLightShadow(i, fragPos, normal);
    vec3 shadowed = getStandardAmbient(spotLights[i].ambient, albedo) * attenuation;

    result += shadow > 0.0
      ? mix(shadowed, StandardSpotLightAffect(spotLights[i], albedo, normal, fragPos, camera.position, 32), shadow)
      : shadowed;
  }

  for (int i = 0; i < numDirectionalLights; i++)
  {
    DirectionalLight light = directionalLights[i];

    float shadow = facesDirectionalLight(normal, light.direction) ? directionalLightShadow(i, fragPos, normal) : 0.0;
    vec3 shadowed = getStandardAmbient(light.ambient, albedo);

    result += shadow > 0.0
      ? mix(shadowed, StandardDirectionalLightAffect(light, albedo, normal, fragPos, camera.position, 32), shadow)
      : shadowed;
  }

  outColor = vec4(result, 1.0);
}
