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

layout(set = 2, binding = 7) uniform sampler2DArrayShadow[] directionalLightShadowMaps;

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

// The fragment in the clip space of cascade c of the directional light: xy mapped to [0, 1], z the depth
vec3 directionalCascadeCoords(int i, int c, vec3 fragPos)
{
  vec4 clip = directionalLights[i].cascadeViewProjections[c] * vec4(fragPos, 1.0);
  vec3 coords = clip.xyz / clip.w;
  coords.xy = coords.xy * 0.5 + 0.5;

  return coords;
}

bool insideUnitCube(vec3 coords)
{
  return all(greaterThanEqual(coords, vec3(0.0))) && all(lessThanEqual(coords, vec3(1.0)));
}

// 1 where the fragment is lit by the directional light, 0 where its shadow map hides it. The map has one layer per
// cascade (DirectionalLight): the first cascade that contains the fragment is sampled, blended with the next near its
// edge, and the shadow fades out over the last tenth of the shadow distance. Fragments beyond it, or in no cascade, are lit.
float directionalLightShadow(int i, vec3 fragPos)
{
  int shadowMap = directionalLights[i].shadowMapIndex;
  int cascadeCount = directionalLights[i].cascadeCount;

  // A light without a map casts no shadows
  if (shadowMap < 0 || cascadeCount == 0)
  {
    return 1.0;
  }

  float shadowDistance = directionalLights[i].shadowDistance;
  float fade = smoothstep(0.9 * shadowDistance, shadowDistance, length(fragPos - camera.position));

  if (fade >= 1.0)
  {
    return 1.0;
  }

  for (int c = 0; c < cascadeCount; c++)
  {
    vec3 coords = directionalCascadeCoords(i, c, fragPos);

    if (!insideUnitCube(coords))
    {
      continue;
    }

    float shadow = texture(directionalLightShadowMaps[nonuniformEXT(shadowMap)],
                           vec4(coords.xy, float(c), coords.z - directionalLights[i].cascadeBiases[c]));

    // Near the edge of the cascade, blend toward the next one so the change of resolution isn't a visible line
    const float blendWidth = 0.05;
    float edge = min(min(coords.x, 1.0 - coords.x), min(coords.y, 1.0 - coords.y));

    if (edge < blendWidth && c + 1 < cascadeCount)
    {
      vec3 nextCoords = directionalCascadeCoords(i, c + 1, fragPos);

      if (insideUnitCube(nextCoords))
      {
        float nextShadow = texture(directionalLightShadowMaps[nonuniformEXT(shadowMap)],
                                   vec4(nextCoords.xy, float(c + 1), nextCoords.z - directionalLights[i].cascadeBiases[c + 1]));

        shadow = mix(nextShadow, shadow, edge / blendWidth);
      }
    }

    return mix(shadow, 1.0, fade);
  }

  return 1.0;
}
