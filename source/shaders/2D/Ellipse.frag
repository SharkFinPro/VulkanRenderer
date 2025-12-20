#version 450

layout(push_constant) uniform EllipsePC {
  layout(offset = 72)
  float x;
  float y;
  float width;
  float height;
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