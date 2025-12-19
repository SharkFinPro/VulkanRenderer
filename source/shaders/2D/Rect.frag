#version 450

layout(push_constant) uniform RectPC {
  layout(offset = 88)
  float r;
  float g;
  float b;
  float a;
} pc;

layout(location = 0) out vec4 outColor;

void main()
{
  outColor = vec4(pc.r, pc.g, pc.b, pc.a);
}