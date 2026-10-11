#version 450

layout(set = 0, binding = 0) uniform Transform {
  mat4 model;
  mat4 view;
  mat4 proj;
} transform;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

void main()
{
  fragTexCoord = inTexCoord;

  gl_Position = transform.proj * transform.view * transform.model * vec4(inPosition, 1.0);
}
