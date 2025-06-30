#version 450

layout(push_constant) uniform PushConstants {
  uint objectID;
};

layout(location = 0) out vec4 outColor;

void main()
{
  float r = float((objectID >> 16) & 0xFF) / 255.0;
  float g = float((objectID >> 8) & 0xFF) / 255.0;
  float b = float((objectID >> 0) & 0xFF) / 255.0;

  outColor = vec4(r, g, b, 1.0);
}