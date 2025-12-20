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

layout(location = 0) in vec2 fragPos;

layout(location = 0) out vec4 outColor;

void main()
{
  // Vector from ellipse center to fragment
  vec2 offsetFromCenter = fragPos - vec2(pc.x, pc.y);

  // Scale factor to normalize ellipse to unit circle
  vec2 ellipseScale = vec2(2.0 / pc.width, 2.0 / pc.height);

  // Transform to normalized space where ellipse becomes a unit circle
  vec2 normalizedPos = offsetFromCenter * ellipseScale;

  // Check if point is inside unit circle (and thus inside ellipse)
  float isInside = step(dot(normalizedPos, normalizedPos), 1.0);

  // Output color with alpha masked by ellipse shape
  outColor = vec4(pc.r, pc.g, pc.b, pc.a * isInside);
}