#version 450
#extension GL_GOOGLE_include_directive : require
#include "common/Lighting.glsl"

layout(set = 1, binding = 0) uniform Transform {
  mat4 model;
  mat4 view;
  mat4 proj;
} transform;

layout(set = 1, binding = 1) uniform sampler2D texSampler;
layout(set = 1, binding = 4) uniform sampler2D specSampler;

layout(set = 0, binding = 0) uniform PointLightsMetadata {
  int numPointLights;
  int numSpotLights;
};

layout(set = 0, binding = 1) readonly buffer PointLights {
  PointLight pointLights[];
};

layout(set = 0, binding = 2) readonly buffer SpotLights {
  SpotLight spotLights[];
};

layout(set = 0, binding = 3) uniform Camera {
  vec3 position;
} camera;

layout(set = 0, binding = 4) uniform sampler2DShadow[16] spotLightShadowMaps;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main()
{
  vec3 texColor = texture(texSampler, fragTexCoord).rgb;
  vec3 specColor = texture(specSampler, fragTexCoord).rgb;

  vec3 result = vec3(0);
  for (int i = 0; i < numPointLights; i++)
  {
    result += SpecularMapPointLightAffect(pointLights[i], texColor, specColor, fragNormal, fragPos, camera.position, 32);
  }

  for (int i = 0; i < numSpotLights; i++)
  {
    vec4 fragPosLightSpace = spotLights[i].lightViewProjection * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    if (projCoords.z > 1.0 ||
    projCoords.x < 0.0 || projCoords.x > 1.0 ||
    projCoords.y < 0.0 || projCoords.y > 1.0)
    {
      // Outside shadow map = not shadowed
      result += getStandardAmbient(spotLights[i].ambient, texColor);
      continue;
    }

    float bias = 0.0001;
    projCoords.z -= bias;

    float shadow = texture(spotLightShadowMaps[i], projCoords);
    bool visible = shadow > 0.5;

    if (visible)
    {
      result += SpecularMapSpotLightAffect(spotLights[i], texColor, specColor, fragNormal, fragPos, camera.position, 32);
    }
    else
    {
      result += getStandardAmbient(spotLights[i].ambient, texColor);
    }
  }

  outColor = vec4(result, 1.0);
}