#version 450

layout(set = 1, binding = 0) uniform Transform {
  mat4 model;
  mat4 view;
  mat4 proj;
} transform;

layout(set = 0, binding = 4) uniform Snake {
  float wiggle;
} snake;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;

void main()
{
  vec3 pos = inPosition;
  pos.z += sin(pos.x * 0.5) * snake.wiggle;

  fragPos = vec3(transform.model * vec4(pos, 1.0));
  fragTexCoord = inTexCoord;
  fragNormal = mat3(transpose(inverse(transform.model))) * inNormal;
}