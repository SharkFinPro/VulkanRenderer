#version 450
#extension GL_GOOGLE_include_directive : require
#include "common/Lighting.glsl"

layout(set = 0, binding = 2) uniform sampler2D texSampler;

layout(set = 1, binding = 0) uniform PointLightsMetadata {
  int numPointLights;
  int numSpotLights;
};

layout(set = 1, binding = 1) readonly buffer PointLights {
  PointLight pointLights[];
};

layout(set = 1, binding = 2) readonly buffer SpotLights {
  SpotLight spotLights[];
};

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

void main()
{
  vec4 texColor = texture(texSampler, fragTexCoord);

  if (texColor.a < 0.1)
  {
    discard;
  }

  vec3 result = vec3(0);
  for (int i = 0; i < numPointLights; i++)
  {
    result += SmokePointLightAffect(pointLights[i], texColor.rgb, fragPos);
  }

  for (int i = 0; i < numSpotLights; i++)
  {
    result += SmokeSpotLightAffect(spotLights[i], texColor.rgb, fragPos);
  }

  outColor = vec4(result, texColor.a);
//  outColor = vec4(texColor.rgb, 1.0);
//  outColor = vec4(0.2, 0.8, 0.2, 1.0);
}