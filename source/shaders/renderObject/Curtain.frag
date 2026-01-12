#version 450
#extension GL_GOOGLE_include_directive : require
#include "../common/Lighting.glsl"

layout(push_constant) uniform CurtainPC {
  float amplitude;
  float period;
  float shininess;
} pc;

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

layout(set = 1, binding = 3) uniform Camera {
  vec3 position;
} camera;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main()
{
  vec3 fragColor = vec3(0.855, 0.647, 0.125);

  // now use fragColor in the per-fragment lighting equations:
  vec3 result = vec3(0);
  for (int i = 0; i < numPointLights; i++)
  {
    result += StandardPointLightAffect(pointLights[i], fragColor, fragNormal, fragPos, camera.position, pc.shininess);
  }

  for (int i = 0; i < numSpotLights; i++)
  {
    result += StandardSpotLightAffect(spotLights[i], fragColor, fragNormal, fragPos, camera.position, pc.shininess);
  }

  outColor = vec4(result, 1.0);
}