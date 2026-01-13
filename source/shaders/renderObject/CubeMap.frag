#version 450
#extension GL_GOOGLE_include_directive : require
#include "../common/Perturb.glsl"

layout(push_constant) uniform CubeMapPC {
  vec3 position;
  float mix;
  float refractionIndex;
  float whiteMix;
  float noiseAmplitude;
  float noiseFrequency;
} pc;

layout(set = 0, binding = 0) uniform Transform {
  mat4 model;
  mat4 view;
  mat4 proj;
} transform;

layout(set = 1, binding = 0) uniform sampler3D Noise3;

layout(set = 1, binding = 1) uniform samplerCube RoomCubeMap;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

const vec3 WHITE = vec3(1.0, 1.0, 1.0);

void main()
{
  vec3 Normal = normalize(fragNormal);
  vec3 Eye = normalize(fragPos - pc.position);

  vec4 nvx = texture(Noise3, pc.noiseFrequency * fragPos);
  vec4 nvy = texture(Noise3, pc.noiseFrequency * vec3(fragPos.xy, fragPos.z + 0.33));
  vec4 nvz = texture(Noise3, pc.noiseFrequency * vec3(fragPos.xy, fragPos.z + 0.67));

  float angx = nvx.r + nvx.g + nvx.b + nvx.a;	//  1. -> 3.
  angx = angx - 2.;				// -1. -> 1.
  angx *= pc.noiseAmplitude;

  float angy = nvy.r + nvy.g + nvy.b + nvy.a;	//  1. -> 3.
  angy = angy - 2.;				// -1. -> 1.
  angy *= pc.noiseAmplitude;

  float angz = nvz.r + nvz.g + nvz.b + nvz.a;	//  1. -> 3.
  angz = angz - 2.;				// -1. -> 1.
  angz *= pc.noiseAmplitude;

  Normal = PerturbNormal3( angx, angy, angz, Normal );
  Normal = normalize(transpose(inverse(mat3(transform.model))) * Normal);

  vec3 reflectVector = reflect(Eye, Normal);
  vec3 reflectColor = texture(RoomCubeMap, reflectVector).rgb;

  vec3 refractVector = refract(Eye, Normal, pc.refractionIndex);

  vec3 refractColor;
  if( all( equal( refractVector, vec3(0.,0.,0.) ) ) )
  {
    refractColor = reflectColor;
  }
  else
  {
    refractColor = texture(RoomCubeMap, refractVector).rgb;
    refractColor = mix(refractColor, WHITE, pc.whiteMix);
  }

  outColor = vec4(mix(refractColor, reflectColor, pc.mix), 1);
}