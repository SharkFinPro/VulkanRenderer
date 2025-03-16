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
  gl_PointSize = 14.0;

  fragPos = vec3(transform.model * vec4(inPosition, 1.0));
  gl_Position = transform.proj * transform.view * transform.model * vec4(inPosition, 1.0);

  fragColor = inColor;
}