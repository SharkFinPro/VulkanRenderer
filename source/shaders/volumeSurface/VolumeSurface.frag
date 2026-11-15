#version 450

layout(push_constant) uniform VolumeSurfacePC {
  mat4 model;
  vec4 normalMatrix[3];
  vec4 color;
} pc;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) flat in uint fragMaterial;

layout(location = 0) out vec4 outColor;

// A placeholder look until volume surfaces get the lit pipelines: a fixed light and a small palette by material.
const vec3 palette[8] = vec3[](
  vec3(0.60, 0.60, 0.60),
  vec3(0.55, 0.75, 0.40),
  vec3(0.70, 0.55, 0.35),
  vec3(0.55, 0.60, 0.80),
  vec3(0.85, 0.80, 0.55),
  vec3(0.80, 0.50, 0.50),
  vec3(0.50, 0.75, 0.75),
  vec3(0.75, 0.60, 0.80)
);

void main()
{
  const vec3 lightDirection = normalize(vec3(0.4, 1.0, 0.3));
  const float lambert = max(dot(normalize(fragNormal), lightDirection), 0.0);
  const vec3 base = pc.color.rgb * palette[fragMaterial % 8];

  outColor = vec4(base * (0.25 + 0.75 * lambert), pc.color.a);
}
