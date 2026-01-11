#version 450
#extension GL_GOOGLE_include_directive : require
#include "../common/Lighting.glsl"

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

layout(push_constant) uniform PushConstants {
  float wiggle;
};

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

vec3 hsvToRgb(float h, float s, float v)
{
  float c = v * s;
  float x = c * (1.0 - abs(mod(h * 6.0, 2.0) - 1.0));
  float m = v - c;

  vec3 rgb;

  if (h < 1.0 / 6.0)      rgb = vec3(c, x, 0.0);
  else if (h < 2.0 / 6.0) rgb = vec3(x, c, 0.0);
  else if (h < 3.0 / 6.0) rgb = vec3(0.0, c, x);
  else if (h < 4.0 / 6.0) rgb = vec3(0.0, x, c);
  else if (h < 5.0 / 6.0) rgb = vec3(x, 0.0, c);
  else                    rgb = vec3(c, 0.0, x);

  return rgb + m;
}

void main()
{
  float pos = (fragPos.x + 11.2);
  float p = pos / 18.0;

  vec3 color = hsvToRgb(mix(1.0 / 6.0, 5.0 / 6.0, p), 1, 1);

  float tension = abs(sin(fragPos.x * 0.5) * wiggle);
  color.g -= tension;
  color.b -= tension;
  color.r += tension;

  vec3 result = vec3(0);
  for (int i = 0; i < numPointLights; i++)
  {
    result += StandardPointLightAffect(pointLights[i], color, fragNormal, fragPos, camera.position, 10);
  }

  for (int i = 0; i < numSpotLights; i++)
  {
    result += StandardSpotLightAffect(spotLights[i], color, fragNormal, fragPos, camera.position, 10);
  }

  outColor = vec4(result, 1.0);
}