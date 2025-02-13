#version 450

layout(set = 0, binding = 3) uniform Camera {
  vec3 position;
} camera;

layout(set = 0, binding = 4) uniform CubeMap {
  float mix;
  float refractionIndex;
  float whiteMix;
} cubeMap;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main()
{
  vec3 fragColor = vec3(0.855, 0.647, 0.125);

  vec3 result = fragColor;

  outColor = vec4(result, 1.0);
}