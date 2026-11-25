// The lighting set (set 2) of the lit object pipelines and the shadow lookups on it. The including shader enables
// GL_EXT_nonuniform_qualifier and GL_GOOGLE_include_directive.
#include "Lighting.glsl"

layout(set = 2, binding = 0) uniform PointLightsMetadata {
  int numPointLights;
  int numSpotLights;
  int numDirectionalLights;
};

layout(set = 2, binding = 1) readonly buffer PointLights {
  PointLight pointLights[];
};

layout(set = 2, binding = 2) readonly buffer SpotLights {
  SpotLight spotLights[];
};

layout(set = 2, binding = 3) uniform Camera {
  vec3 position;
} camera;

layout(set = 2, binding = 4) uniform sampler2DShadow[] spotLightShadowMaps;

layout(set = 2, binding = 5) uniform samplerCubeShadow[] pointLightShadowMaps;

layout(set = 2, binding = 6) readonly buffer DirectionalLights {
  DirectionalLight directionalLights[];
};

layout(set = 2, binding = 7) uniform sampler2DShadow[] directionalLightShadowMaps;

// 1 where the fragment is lit by the point light, 0 where its shadow map hides it
float pointLightShadow(int i, vec3 fragPos)
{
  int shadowMap = pointLights[i].shadowMapIndex;

  // A light without a map casts no shadows
  if (shadowMap < 0)
  {
    return 1.0;
  }

  vec3 fragToLight = pointLights[i].position - fragPos;
  fragToLight.xz *= -1.0;

  float range = pointLights[i].shadowRange;
  float nearPlane = pointLights[i].shadowNearPlane;
  float dist = length(fragToLight);

  // The shadow casters are culled to the range's sphere, so nothing beyond it can hide the fragment
  if (dist >= range)
  {
    return 1.0;
  }

  // The face the lookup reads is the one of the major axis, and that face's view depth is the distance along it. The bias
  // is a world distance along the ray, so it doesn't grow with the range or the angle.
  const float bias = 0.1;
  vec3 absToLight = abs(fragToLight);
  float viewDepth = max(absToLight.x, max(absToLight.y, absToLight.z));
  float biasedDepth = max(viewDepth * (dist - bias) / dist, nearPlane);

  // The reversed depth the face's projection gives at that view depth (PointLight::getLightViewProjectionMatrices). The
  // sampler compares greater-or-equal: lit where the fragment is at least as near as what the map holds.
  float ref = nearPlane * (range - biasedDepth) / (biasedDepth * (range - nearPlane));

  return texture(pointLightShadowMaps[nonuniformEXT(shadowMap)], vec4(fragToLight, ref));
}

float spotLightShadow(int i, vec3 fragPos)
{
  int shadowMap = spotLights[i].shadowMapIndex;

  // A light without a map casts no shadows
  if (shadowMap < 0)
  {
    return 1.0;
  }

  vec4 fragPosLightSpace = spotLights[i].lightViewProjection * vec4(fragPos, 1.0);
  vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
  projCoords.xy = projCoords.xy * 0.5 + 0.5;

  // Beyond the far plane (the shadow range) nothing is in the map, so the fragment is lit
  if (projCoords.z > 1.0)
  {
    return 1.0;
  }

  float bias = 0.00005;
  projCoords.z -= bias;

  return texture(spotLightShadowMaps[nonuniformEXT(shadowMap)], projCoords);
}

// 1 where the fragment is lit by the directional light, 0 where its shadow map hides it. The map covers only the box the
// light is fitted to (DirectionalLight); fragments outside it are lit.
float directionalLightShadow(int i, vec3 fragPos)
{
  int shadowMap = directionalLights[i].shadowMapIndex;

  // A light without a map casts no shadows
  if (shadowMap < 0)
  {
    return 1.0;
  }

  vec4 fragPosLightSpace = directionalLights[i].lightViewProjection * vec4(fragPos, 1.0);
  vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
  projCoords.xy = projCoords.xy * 0.5 + 0.5;

  if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
      projCoords.y < 0.0 || projCoords.y > 1.0 ||
      projCoords.z < 0.0 || projCoords.z > 1.0)
  {
    return 1.0;
  }

  projCoords.z -= directionalLights[i].shadowBias;

  return texture(directionalLightShadowMaps[nonuniformEXT(shadowMap)], projCoords);
}
