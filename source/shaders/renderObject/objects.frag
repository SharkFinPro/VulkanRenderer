#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#include "../common/SceneLighting.glsl"

layout(set = 1, binding = 0) uniform sampler2D texSampler;
layout(set = 1, binding = 1) uniform sampler2D specSampler;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
// Written by the shared vertex shader; only some fragment shaders need the object
layout(location = 3) flat in uint fragObject;

layout(location = 0) out vec4 outColor;

void main()
{
  vec3 texColor = texture(texSampler, fragTexCoord).rgb;
  vec3 specColor = texture(specSampler, fragTexCoord).rgb;

  vec3 shadowNormal = normalize(fragNormal);

  vec3 result = vec3(0);
  for (int i = 0; i < numPointLights; i++)
  {
    PointLight light = pointLights[i];

    float attenuation = lightAttenuation(light.range, light.position, fragPos);
    if (attenuation <= 0.0)
    {
      continue;
    }

    float shadow = facesLight(fragNormal, fragPos, light.position) ? pointLightShadow(i, fragPos, shadowNormal) : 0.0;
    vec3 shadowed = getStandardAmbient(light.ambient, texColor) * attenuation;

    // The shadow is a fraction, so the edge of a shadow blends from one result to the other
    result += shadow > 0.0
      ? mix(shadowed, SpecularMapPointLightAffect(light, texColor, specColor, fragNormal, fragPos, camera.position, 32), shadow)
      : shadowed;
  }

  for (int i = 0; i < numSpotLights; i++)
  {
    float attenuation = lightAttenuation(spotLights[i].range, spotLights[i].position, fragPos);
    if (attenuation <= 0.0)
    {
      continue;
    }

    float shadow = facesLight(fragNormal, fragPos, spotLights[i].position) ? spotLightShadow(i, fragPos, shadowNormal) : 0.0;
    vec3 shadowed = getStandardAmbient(spotLights[i].ambient, texColor) * attenuation;

    result += shadow > 0.0
      ? mix(shadowed, SpecularMapSpotLightAffect(spotLights[i], texColor, specColor, fragNormal, fragPos, camera.position, 32), shadow)
      : shadowed;
  }

  for (int i = 0; i < numDirectionalLights; i++)
  {
    DirectionalLight light = directionalLights[i];

    float shadow = facesDirectionalLight(fragNormal, light.direction) ? directionalLightShadow(i, fragPos, shadowNormal) : 0.0;
    vec3 shadowed = getStandardAmbient(light.ambient, texColor);

    result += shadow > 0.0
      ? mix(shadowed, SpecularMapDirectionalLightAffect(light, texColor, specColor, fragNormal, fragPos, camera.position, 32), shadow)
      : shadowed;
  }

  outColor = vec4(result, 1.0);
}
