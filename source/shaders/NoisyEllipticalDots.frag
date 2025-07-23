#version 450
#extension GL_GOOGLE_include_directive : require
#include "common/Lighting.glsl"

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

layout(set = 0, binding = 4) uniform EllipticalDots {
  float shininess;
  float sDiameter;
  float tDiameter;
  float blendFactor;
} ellipticalDots;

layout(set = 0, binding = 6) uniform NoiseOptions {
  float amplitude;
  float frequency;
} noiseOptions;

layout(set = 0, binding = 7) uniform sampler3D Noise3;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

const vec3 OBJECTCOLOR = vec3(1, 1, 1);
const vec3 ELLIPSECOLOR = vec3(0.6235, 0.8863, 0.7490);

void main()
{
  vec4 nv = texture(Noise3, noiseOptions.frequency * fragPos);

  float n = nv.r + nv.g + nv.b + nv.a;
  n -= 2.0;
  n *= noiseOptions.amplitude;

  int numins = int(fragTexCoord.s / ellipticalDots.sDiameter);
  int numint = int(fragTexCoord.t / ellipticalDots.tDiameter);

  // Calculate ellipse equation
  float Ar = ellipticalDots.sDiameter / 2.0;
  float Br = ellipticalDots.tDiameter / 2.0;

  float sc = numins * ellipticalDots.sDiameter + Ar;
  float tc = numint * ellipticalDots.tDiameter + Br;

  //
  float ds = fragTexCoord.s - sc;
  float dt = fragTexCoord.t - tc;

  float oldDist = sqrt(ds * ds + dt * dt);
  float scale = (oldDist + n) / oldDist;

  float dist = pow((ds * scale) / Ar, 2.0) + pow((dt * scale) / Br, 2.0);

  // Smooth blending based on ellipse distance
  float t = smoothstep(1.0 - ellipticalDots.blendFactor, 1.0 + ellipticalDots.blendFactor, dist);
  vec3 fragColor = mix(ELLIPSECOLOR, OBJECTCOLOR, t);

  // now use fragColor in the per-fragment lighting equations:
  vec3 result = vec3(0);
  for (int i = 0; i < numPointLights; i++)
  {
    result += StandardPointLightAffect(pointLights[i], fragColor, fragNormal, fragPos, camera.position, ellipticalDots.shininess);
  }

  for (int i = 0; i < numSpotLights; i++)
  {
    result += StandardSpotLightAffect(spotLights[i], fragColor, fragNormal, fragPos, camera.position, ellipticalDots.shininess);
  }

  outColor = vec4(result, 1.0);
}