#version 450
#extension GL_EXT_multiview : require

layout(push_constant) uniform PushConstants {
  vec3 lightPos;
} pc;

layout(location = 0) in vec3 fragPos;

void main() {
  float dist = length(fragPos - pc.lightPos);

  const float farPlane = 100.0;

  float depth = clamp(dist / farPlane, 0.0, 1.0);

  gl_FragDepth = depth;
}