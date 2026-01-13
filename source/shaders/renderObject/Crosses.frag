#version 450
#extension GL_GOOGLE_include_directive : require
#include "../common/Lighting.glsl"

layout(push_constant) uniform CrossesPC {
  vec3 position;
  float quantize;
  float size;
  float shininess;
  float blueDepth;
  float redDepth;
  int level;
  uint useChromaDepth;
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
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in float fragZ;

layout(location = 0) out vec4 outColor;

vec3 Rainbow(float t);

void main()
{
  vec3 color = vec3(0.8);

  if (pc.useChromaDepth == 1)
  {
    float t = (2.0 / 3.0) * (abs(fragZ) - pc.redDepth) / (pc.blueDepth - pc.redDepth);
    t = clamp(t, 0., 2./3.);
    color = Rainbow(t);
  }

  vec3 result = vec3(0);
  for (int i = 0; i < numPointLights; i++)
  {
    result += StandardPointLightAffect(pointLights[i], color, fragNormal, fragPos, camera.position, pc.shininess);
  }

  for (int i = 0; i < numSpotLights; i++)
  {
    result += StandardSpotLightAffect(spotLights[i], color, fragNormal, fragPos, camera.position, pc.shininess);
  }

  outColor = vec4(result, 1.0);
}

vec3 Rainbow(float t)
{
  t = clamp( t, 0., 1. );         // 0.00 is red, 0.33 is green, 0.67 is blue

  float r = 1.;
  float g = 0.0;
  float b = 1.  -  6. * ( t - (5./6.) );

  if( t <= (5./6.) )
  {
    r = 6. * ( t - (4./6.) );
    g = 0.;
    b = 1.;
  }

  if( t <= (4./6.) )
  {
    r = 0.;
    g = 1.  -  6. * ( t - (3./6.) );
    b = 1.;
  }

  if( t <= (3./6.) )
  {
    r = 0.;
    g = 1.;
    b = 6. * ( t - (2./6.) );
  }

  if( t <= (2./6.) )
  {
    r = 1.  -  6. * ( t - (1./6.) );
    g = 1.;
    b = 0.;
  }

  if( t <= (1./6.) )
  {
    r = 1.;
    g = 6. * t;
  }

  return vec3( r, g, b );
}
