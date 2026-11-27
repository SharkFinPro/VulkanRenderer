#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_texture_shadow_lod : require
#extension GL_EXT_control_flow_attributes : require
#include "../common/SceneLighting.glsl"
#include "../common/VolumeMaterial.glsl"

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) flat in uvec4 fragMaterials;
layout(location = 3) in vec4 fragWeights;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2DArray albedoArray;
layout(set = 1, binding = 1) uniform sampler2DArray specularArray;

layout(set = 1, binding = 2) uniform VolumePalette
{
  vec4 textureScales[16];
  uint materialCount;
} palette;

void main()
{
  // Before any branch, so the derivatives are well defined; the sampling below uses explicit gradients
  const vec3 dPdx = dFdx(fragPos);
  const vec3 dPdy = dFdy(fragPos);

  const vec3 axisWeights = triplanarWeights(normalize(fragNormal));

  vec3 albedo = vec3(0.0);
  vec3 specColor = vec3(0.0);
  float totalWeight = 0.0;

  for (int slot = 0; slot < 4; slot++)
  {
    const float materialWeight = fragWeights[slot];
    if (materialWeight <= 0.0)
    {
      continue;
    }

    const float layer = float(fragMaterials[slot] % palette.materialCount);
    const float scale = palette.textureScales[int(layer)].x;

    for (int axis = 0; axis < 3; axis++)
    {
      if (axisWeights[axis] <= 0.0)
      {
        continue;
      }

      const vec2 uv = projectAxis(axis, fragPos) * scale;
      const vec2 gradX = projectAxis(axis, dPdx) * scale;
      const vec2 gradY = projectAxis(axis, dPdy) * scale;
      const float weight = materialWeight * axisWeights[axis];

      albedo += weight * textureGrad(albedoArray, vec3(uv, layer), gradX, gradY).rgb;
      specColor += weight * textureGrad(specularArray, vec3(uv, layer), gradX, gradY).rgb;
    }

    totalWeight += materialWeight;
  }

  // A vertex with no weight at all (not produced by the extractor) falls back to white
  albedo = totalWeight > 0.0 ? albedo / totalWeight : vec3(1.0);
  specColor = totalWeight > 0.0 ? specColor / totalWeight : vec3(1.0);

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
      ? mix(shadowed, SpecularMapPointLightAffect(pointLights[i], albedo, specColor, normal, fragPos, camera.position, 32), shadow)
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
      ? mix(shadowed, SpecularMapSpotLightAffect(spotLights[i], albedo, specColor, normal, fragPos, camera.position, 32), shadow)
      : shadowed;
  }

  for (int i = 0; i < numDirectionalLights; i++)
  {
    DirectionalLight light = directionalLights[i];

    float shadow = facesDirectionalLight(normal, light.direction) ? directionalLightShadow(i, fragPos, normal) : 0.0;
    vec3 shadowed = getStandardAmbient(light.ambient, albedo);

    result += shadow > 0.0
      ? mix(shadowed, SpecularMapDirectionalLightAffect(light, albedo, specColor, normal, fragPos, camera.position, 32), shadow)
      : shadowed;
  }

  outColor = vec4(result, 1.0);
}
