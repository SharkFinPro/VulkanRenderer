#version 450

layout(push_constant) uniform GridPC {
  mat4 viewProj;
  vec3 viewPosition;
} pc;

layout(location = 0) out vec3 nearPoint;
layout(location = 1) out vec3 farPoint;

vec2 positions[4] = vec2[](
  vec2(-1, -1),
  vec2(-1, 1),
  vec2(1, -1),
  vec2(1, 1)
);

vec3 unprojectPoint(vec2 p, float z)
{
  vec4 newPoint = inverse(pc.viewProj) * vec4(p.x, p.y, z, 1.0);

  return newPoint.xyz / newPoint.w;
}

void main() {
  vec2 p = positions[gl_VertexIndex].xy;

  nearPoint = unprojectPoint(p, 0.0);
  farPoint = unprojectPoint(p, 1.0);

  gl_Position = vec4(p, 0.0, 1.0);
}