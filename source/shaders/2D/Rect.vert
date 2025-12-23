#version 450

layout(push_constant) uniform RectPC {
  mat4 transform;
  int screenWidth;
  int screenHeight;
  float z;
  float x;
  float y;
  float width;
  float height;
} pc;

void main()
{
  vec2 pos = vec2(0, 0);
  if (gl_VertexIndex == 0)
  {
    pos = vec2(pc.x, pc.y);
  }
  else if (gl_VertexIndex == 1)
  {
    pos = vec2(pc.x + pc.width, pc.y);
  }
  else if (gl_VertexIndex == 2)
  {
    pos = vec2(pc.x, pc.y + pc.height);
  }
  else
  {
    pos = vec2(pc.x + pc.width, pc.y + pc.height);
  }

  pos = (pc.transform * vec4(pos, 0.0, 1.0)).xy;

  vec2 ndc;
  ndc.x = 2.0 * pos.x / float(pc.screenWidth)  - 1.0;
  ndc.y = 2.0 * pos.y / float(pc.screenHeight) - 1.0;

  gl_Position = vec4(ndc, pc.z, 1.0);
}