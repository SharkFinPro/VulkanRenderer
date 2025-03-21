#version 450

struct PointLight {
  vec3 position;
  float padding1; // Padding to ensure alignment
  vec3 color;
  float padding2; // Padding to ensure alignment
  float ambient;
  float diffuse;
  float specular;
  float padding3; // Padding to ensure alignment
};

layout(set = 0, binding = 5) uniform PointLightsMetadata {
  int numLights;
};

layout(set = 0, binding = 6) readonly buffer PointLights {
  PointLight lights[];
};

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

vec3 PointLightAffect(PointLight light, vec3 color)
{
  // Calculate distance
  vec3 lightToFrag = light.position - fragPos;
  float dist = length(lightToFrag);

  // Calculate attenuation
  float attenuation = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);

  // Combined Output
  return (light.ambient + light.diffuse) * light.color * light.color * attenuation; // Color * Color for brighter color
}


float noise2D(vec2 st)
{
  vec2 i = floor(st);
  vec2 f = fract(st);

  float a = fract(sin(dot(i, vec2(12.9898, 78.233))) * 43758.5453);
  float b = fract(sin(dot(i + vec2(1.0, 0.0), vec2(12.9898, 78.233))) * 43758.5453);
  float c = fract(sin(dot(i + vec2(0.0, 1.0), vec2(12.9898, 78.233))) * 43758.5453);
  float d = fract(sin(dot(i + vec2(1.0, 1.0), vec2(12.9898, 78.233))) * 43758.5453);

  vec2 u = f * f * (3.0 - 2.0 * f);
  return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

void main()
{
  vec2 coord = gl_PointCoord - vec2(0.5);
  float r = length(coord);

  if (r > 0.5)
  {
    discard;
  }

  vec2 noiseCoord = gl_PointCoord * 2.0;
  float noise = noise2D(noiseCoord);
  float secondNoise = noise2D(noiseCoord * 1.5 + vec2(0.2, 0.7));

  float swirl = noise2D(vec2(r * 4.0, atan(coord.y, coord.x) * 2.0));

  float finalMask = (0.7 + 0.3 * noise) * (0.8 + 0.2 * swirl);
  finalMask *= 0.9 + 0.1 * secondNoise;

  vec3 result = vec3(0);
  for (int i = 0; i < numLights; i++)
  {
    result += PointLightAffect(lights[i], fragColor.rgb);
  }

  outColor = vec4(result, finalMask * fragColor.a);
}