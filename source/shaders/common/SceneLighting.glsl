// The lighting set (set 2) of the lit object pipelines and the shadow lookups on it. The including shader enables
// GL_EXT_nonuniform_qualifier and GL_GOOGLE_include_directive.
#include "Lighting.glsl"

layout(set = 2, binding = 0) uniform PointLightsMetadata {
  int numPointLights;
  int numSpotLights;
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

// 1 where the fragment is lit by the point light, 0 where its shadow map hides it
float pointLightShadow(int i, vec3 fragPos)
{
  vec3 fragToLight = pointLights[i].position - fragPos;
  fragToLight.xz *= -1.0;

  float currentDist = length(fragToLight);
  float ref = currentDist / 100.0;

  float bias = 0.001;
  ref -= bias;

  return texture(pointLightShadowMaps[nonuniformEXT(i)], vec4(fragToLight, ref));
}

float spotLightShadow(int i, vec3 fragPos)
{
  vec4 fragPosLightSpace = spotLights[i].lightViewProjection * vec4(fragPos, 1.0);
  vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
  projCoords.xy = projCoords.xy * 0.5 + 0.5;

  float bias = 0.00005;
  projCoords.z -= bias;

  return texture(spotLightShadowMaps[nonuniformEXT(i)], projCoords);
}
