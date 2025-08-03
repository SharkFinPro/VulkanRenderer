#version 450
#extension GL_GOOGLE_include_directive : require
#include "common/Lighting.glsl"

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

layout(push_constant) uniform PushConstants {
  float shininess;
  float sDiameter;
  float tDiameter;
  float blendFactor;
};

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

const vec3 OBJECTCOLOR = vec3(1, 1, 1);
const vec3 ELLIPSECOLOR = vec3(0.6235, 0.8863, 0.7490);

void main()
{
  // blend OBJECTCOLOR and ELLIPSECOLOR by using the ellipse equation to decide how close
  // 	this fragment is to the ellipse border:

  int numins = int(fragTexCoord.s / sDiameter);
  int numint = int(fragTexCoord.t / tDiameter);

  // Calculate ellipse equation
  float Ar = sDiameter / 2.0;
  float Br = tDiameter / 2.0;

  float sc = numins * sDiameter + Ar;
  float tc = numint * tDiameter + Br;

  float dist = pow((fragTexCoord.s - sc) / Ar, 2.0) + pow((fragTexCoord.t - tc) / Br, 2.0);

  // Smooth blending based on ellipse distance
  float t = smoothstep(1.0 - blendFactor, 1.0 + blendFactor, dist);
  vec3 fragColor = mix(ELLIPSECOLOR, OBJECTCOLOR, t);

  // now use fragColor in the per-fragment lighting equations:
  vec3 result = vec3(0);
  for (int i = 0; i < numPointLights; i++)
  {
    result += StandardPointLightAffect(pointLights[i], fragColor, fragNormal, fragPos, camera.position, shininess);
  }

  for (int i = 0; i < numSpotLights; i++)
  {
    result += StandardSpotLightAffect(spotLights[i], fragColor, fragNormal, fragPos, camera.position, shininess);
  }

  outColor = vec4(result, 1.0);
}