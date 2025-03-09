#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 gsPos;
layout(location = 1) out vec3 gsNormal;

void main()
{
  gsPos = inPosition;
  gsNormal = inNormal;
}