#version 450
#extension GL_GOOGLE_include_directive : require
#include "../common/SceneData.glsl"

layout(push_constant) uniform VolumeSurfacePC {
  mat4 model;
  vec4 normalMatrix[3];
  vec4 color;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in uint inMaterial;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) flat out uint fragMaterial;

void main()
{
  const mat3 normalMatrix = mat3(pc.normalMatrix[0].xyz, pc.normalMatrix[1].xyz, pc.normalMatrix[2].xyz);

  fragNormal = normalMatrix * inNormal;
  fragMaterial = inMaterial;

  gl_Position = scene.proj * scene.view * pc.model * vec4(inPosition, 1.0);
}
