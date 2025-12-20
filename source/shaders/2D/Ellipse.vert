#version 450

layout(push_constant) uniform EllipsePC {
  mat4 transform;
  int screenWidth;
  int screenHeight;
  float x;
  float y;
  float width;
  float height;
  float r;
  float g;
  float b;
  float a;
} pc;

void main()
{
  vec2 pos = vec2(0, 0);
  if (gl_VertexIndex == 0)
  {
    pos = vec2(pc.x - pc.width / 2.0, pc.y - pc.height / 2.0);
  }
  else if (gl_VertexIndex == 1)
  {
    pos = vec2(pc.x + pc.width / 2.0, pc.y - pc.height / 2.0);
  }
  else if (gl_VertexIndex == 2)
  {
    pos = vec2(pc.x - pc.width / 2.0, pc.y + pc.height / 2.0);
  }
  else
  {
    pos = vec2(pc.x + pc.width / 2.0, pc.y + pc.height / 2.0);
  }

  pos = (pc.transform * vec4(pos, 0.0, 1.0)).xy;

  vec2 ndc;
  ndc.x = 2.0 * pos.x / float(pc.screenWidth)  - 1.0;
  ndc.y = 2.0 * pos.y / float(pc.screenHeight) - 1.0;

  gl_Position = vec4(ndc, 0.0, 1.0);
}