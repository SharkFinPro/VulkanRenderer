#version 450

layout(push_constant) uniform QuadPC {
  mat4 transformation;
  vec2 screenSize;
  vec4 bounds;
} pc;

void main()
{
  vec2 pos = vec2(0, 0);
  if (gl_VertexIndex == 0)
  {
    pos = vec2(pc.bounds.r, pc.bounds.g);
  }
  else if (gl_VertexIndex == 1)
  {
    pos = vec2(pc.bounds.r + pc.bounds.b, pc.bounds.g);
  }
  else if (gl_VertexIndex == 2)
  {
    pos = vec2(pc.bounds.r, pc.bounds.g + pc.bounds.a);
  }
  else
  {
    pos = vec2(pc.bounds.r + pc.bounds.b, pc.bounds.g + pc.bounds.a);
  }

  pos = (pc.transformation * vec4(pos, 0.0, 1.0)).xy;

  vec2 ndc;
  ndc.x = 2.0 * pos.x / float(pc.screenSize.x)  - 1.0;
  ndc.y = 2.0 * pos.y / float(pc.screenSize.y) - 1.0;

  gl_Position = vec4(ndc, 0.0, 1.0);
}