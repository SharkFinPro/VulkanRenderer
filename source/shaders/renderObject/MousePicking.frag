#version 450

layout(push_constant) uniform PushConstants {
  uint objectID;
};

layout(location = 0) out uvec4 outColor;

void main()
{
  float r = (objectID >> 16) & 0xFF;
  float g = (objectID >> 8) & 0xFF;
  float b = (objectID >> 0) & 0xFF;

  outColor = uvec4(r, g, b, 255);
}