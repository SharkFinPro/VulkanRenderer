#version 450
#extension GL_GOOGLE_include_directive : require
#include "common/Lighting.glsl"
#include "common/Perturb.glsl"

layout(set = 1, binding = 0) uniform Transform {
  mat4 model;
  mat4 view;
  mat4 proj;
} transform;

layout(set = 2, binding = 2) uniform PointLightsMetadata {
  int numLights;
};

layout(set = 2, binding = 5) readonly buffer PointLights {
  PointLight lights[];
};

layout(set = 2, binding = 3) uniform Camera {
  vec3 position;
} camera;

layout(set = 0, binding = 4) uniform Curtain {
  float amplitude;
  float period;
  float shininess;
} curtain;

layout(set = 0, binding = 6) uniform NoiseOptions {
  float amplitude;
  float frequency;
} noiseOptions;

layout(set = 0, binding = 7) uniform sampler3D Noise3;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main()
{
  vec4 nvx = texture(Noise3, noiseOptions.frequency * fragPos);
  float angx = nvx.r + nvx.g + nvx.b + nvx.a  -  2.;	// -1. to +1.
  angx *= noiseOptions.amplitude;

  vec4 nvy = texture(Noise3, noiseOptions.frequency * vec3(fragPos.xy, fragPos.z + 0.5));
  float angy = nvy.r + nvy.g + nvy.b + nvy.a  -  2.;	// -1. to +1.
  angy *= noiseOptions.amplitude;

  vec3 n = PerturbNormal2(angx, angy, fragNormal);
  n = normalize(transpose(inverse(mat3(transform.model))) * n);

  vec3 fragColor = vec3(1, 1, 1);

  // now use fragColor in the per-fragment lighting equations:
  vec3 result = vec3(0);
  for (int i = 0; i < numLights; i++)
  {
    result += StandardPointLightAffect(lights[i], fragColor, n, fragPos, camera.position, curtain.shininess);
  }

  outColor = vec4(result, 1.0);
}