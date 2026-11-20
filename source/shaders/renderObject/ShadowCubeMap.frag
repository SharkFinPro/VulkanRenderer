#version 450
#extension GL_EXT_multiview : require

layout(push_constant) uniform PushConstants {
  vec3 lightPos;
  float shadowRange;
} pc;

layout(location = 0) in vec3 fragPos;

void main() {
  float dist = length(fragPos - pc.lightPos);

  float depth = clamp(dist / pc.shadowRange, 0.0, 1.0);

  gl_FragDepth = depth;
}