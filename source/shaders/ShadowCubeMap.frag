#version 450
#extension GL_EXT_multiview : require

layout(set = 1, binding = 1) uniform Light {
  vec3 position;
} light;

layout(location = 0) in vec3 fragPos;

const float FAR_PLANE = 100.0;

void main() {
  float lightDistance = length(fragPos - light.position);

  lightDistance = lightDistance / FAR_PLANE;

  gl_FragDepth = lightDistance;
}