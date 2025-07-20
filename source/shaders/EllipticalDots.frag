#version 450
#extension GL_GOOGLE_include_directive : require
#include "common/Lighting.glsl"

layout(set = 0, binding = 2) uniform PointLightsMetadata {
  int numLights;
};

layout(set = 0, binding = 5) readonly buffer PointLights {
  PointLight lights[];
};

layout(set = 0, binding = 3) uniform Camera {
  vec3 position;
} camera;

layout(set = 0, binding = 4) uniform EllipticalDots {
  float shininess;
  float sDiameter;
  float tDiameter;
  float blendFactor;
} ellipticalDots;

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

  int numins = int(fragTexCoord.s / ellipticalDots.sDiameter);
  int numint = int(fragTexCoord.t / ellipticalDots.tDiameter);

  // Calculate ellipse equation
  float Ar = ellipticalDots.sDiameter / 2.0;
  float Br = ellipticalDots.tDiameter / 2.0;

  float sc = numins * ellipticalDots.sDiameter + Ar;
  float tc = numint * ellipticalDots.tDiameter + Br;

  float dist = pow((fragTexCoord.s - sc) / Ar, 2.0) + pow((fragTexCoord.t - tc) / Br, 2.0);

  // Smooth blending based on ellipse distance
  float t = smoothstep(1.0 - ellipticalDots.blendFactor, 1.0 + ellipticalDots.blendFactor, dist);
  vec3 fragColor = mix(ELLIPSECOLOR, OBJECTCOLOR, t);

  // now use fragColor in the per-fragment lighting equations:
  vec3 result = vec3(0);
  for (int i = 0; i < numLights; i++)
  {
    result += StandardPointLightAffect(lights[i], fragColor, fragNormal, fragPos, camera.position, ellipticalDots.shininess);
  }

  outColor = vec4(result, 1.0);
}