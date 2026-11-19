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

  vec3 result = vec3(0);
  for (int i = 0; i < numPointLights; i++)
  {
    PointLight light = pointLights[i];

    if (pointLightShadow(i, fragPos) > 0.1)
    {
      result += SpecularMapPointLightAffect(light, texColor, specColor, fragNormal, fragPos, camera.position, 32);
    }
    else
    {
      result += getStandardAmbient(light.ambient, texColor);
    }
  }

  for (int i = 0; i < numSpotLights; i++)
  {
    if (spotLightShadow(i, fragPos) > 0.5)
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
