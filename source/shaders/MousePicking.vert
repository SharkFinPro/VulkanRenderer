#version 450

layout(set = 0, binding = 0) uniform Transform {
  mat4 model;
  mat4 view;
  mat4 proj;
} transform;

layout(location = 0) in vec3 inPosition;

void main()
{
  gl_Position = transform.proj * transform.view * transform.model * vec4(inPosition, 1.0);
}