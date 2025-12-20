#version 450

layout(push_constant) uniform TrianglePC {
  layout(offset = 16)
  mat4 transform;
  int screenWidth;
  int screenHeight;
  float x1;
  float y1;
  float x2;
  float y2;
  float x3;
  float y3;
} pc;

void main()
{
  vec2 pos = vec2(0, 0);
  if (gl_VertexIndex == 0)
  {
    pos = vec2(pc.x1, pc.y1);
  }
  else if (gl_VertexIndex == 1)
  {
    pos = vec2(pc.x2, pc.y2);
  }
  else
  {
    pos = vec2(pc.x3, pc.y3);
  }

  pos = (pc.transform * vec4(pos, 0.0, 1.0)).xy;

  vec2 ndc;
  ndc.x = 2.0 * pos.x / float(pc.screenWidth)  - 1.0;
  ndc.y = 2.0 * pos.y / float(pc.screenHeight) - 1.0;

  gl_Position = vec4(ndc, 0.0, 1.0);
}