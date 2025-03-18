#version 450

layout(set = 0, binding = 3) uniform Transform {
  mat4 model;
  mat4 view;
  mat4 proj;
} transform;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec4 fragColor;

void main()
{
  vec4 viewPos = transform.view * transform.model * vec4(inPosition, 1.0);

  float basePointSize = 7.0;
  float distanceScale = 1.0 / -viewPos.z;

  gl_PointSize = clamp(basePointSize * distanceScale, 1.0, 20.0);
  gl_Position = transform.proj * viewPos;

  fragColor = inColor;
  fragPos = vec3(transform.model * vec4(inPosition, 1.0));
}