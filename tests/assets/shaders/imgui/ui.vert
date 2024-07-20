#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;

void main()
{
  fragPos = inPosition;
  fragTexCoord = inTexCoord;
  fragNormal = inNormal;

  gl_Position = vec4(inPosition, 1.0);
}