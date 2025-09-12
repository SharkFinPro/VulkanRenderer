#version 450

layout(set = 0, binding = 0) uniform Grid {
  mat4 view;
  mat4 proj;
} grid;

layout(location = 0) in vec3 nearPoint;
layout(location = 1) in vec3 farPoint;

layout(location = 0) out vec4 outColor;

void main() {
  float t = -nearPoint.y / (farPoint.y - nearPoint.y);

  outColor = vec4(0.2, 0.3, 0.8, 1.0 * float(t > 0));
}