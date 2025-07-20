#version 450
#extension GL_GOOGLE_include_directive : require
#include "common/structs.glsl"

layout(set = 0, binding = 2) uniform PointLightsMetadata {
  int numLights;
};

layout(set = 0, binding = 5) readonly buffer PointLights {
  PointLight lights[];
};

layout(set = 0, binding = 3) uniform Camera {
  vec3 position;
} camera;

layout(set = 0, binding = 4) uniform Crosses {
  int level;
  float quantize;
  float size;
  float shininess;
} crosses;

layout(std140, set = 0, binding = 6) uniform ChromaDepth {
  bool use;
  float blueDepth;
  float redDepth;
} chromaDepth;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in float fragZ;

layout(location = 0) out vec4 outColor;

vec3 Rainbow(float t);

vec3 PointLightAffect(PointLight light, vec3 color);

void main()
{
  vec3 color = vec3(0.8);

  if (chromaDepth.use)
  {
    float t = (2.0 / 3.0) * (abs(fragZ) - chromaDepth.redDepth) / (chromaDepth.blueDepth - chromaDepth.redDepth);
    t = clamp(t, 0., 2./3.);
    color = Rainbow(t);
  }

  vec3 result = vec3(0);
  for (int i = 0; i < numLights; i++)
  {
    result += PointLightAffect(lights[i], color);
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

vec3 PointLightAffect(PointLight light, vec3 color)
{
  // Ambient
  vec3 ambient = light.ambient * color;

  // Diffuse
  vec3 norm = normalize(fragNormal);
  vec3 lightDir = normalize(light.position - fragPos);
  float d = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = light.diffuse * d * color;

  // Specular
  vec3 specular = vec3(0);
  if(d > 0.0) // only do specular if the light can see the point
  {
    vec3 viewDir = normalize(camera.position - fragPos);
    vec3 reflectDir = normalize(reflect(-lightDir, norm));
    float cosphi = dot(viewDir, reflectDir);

    if (cosphi > 0.0)
    {
      specular = pow(cosphi, crosses.shininess) * light.specular * light.color;
    }
  }

  // Combined Output
  return (ambient + diffuse + specular) * light.color;
}