#version 450

layout(set = 0, binding = 0) uniform usampler2D outlineMask;

layout(set = 1, binding = 0) uniform OutlineColors {
  vec4 colors[256];
};

layout(push_constant) uniform PushConstants {
  int width;
};

layout(location = 0) out vec4 outColor;

void main()
{
  const ivec2 pixel = ivec2(gl_FragCoord.xy);
  const ivec2 size = textureSize(outlineMask, 0);

  // Inside an outlined object, which is drawn as it is
  if (texelFetch(outlineMask, pixel, 0).r != 0u)
  {
    discard;
  }

  // The closest outlined pixel within a disc of the requested width gives the color, so the outline is round at
  // corners and two touching colors meet halfway
  uint closestValue = 0u;
  int closestDistance = width * width + 1;

  for (int y = -width; y <= width; ++y)
  {
    for (int x = -width; x <= width; ++x)
    {
      const int distanceSquared = x * x + y * y;
      if (distanceSquared >= closestDistance)
      {
        continue;
      }

      const ivec2 neighbor = pixel + ivec2(x, y);
      if (any(lessThan(neighbor, ivec2(0))) || any(greaterThanEqual(neighbor, size)))
      {
        continue;
      }

      const uint value = texelFetch(outlineMask, neighbor, 0).r;
      if (value != 0u)
      {
        closestValue = value;
        closestDistance = distanceSquared;
      }
    }
  }

  if (closestValue == 0u)
  {
    discard;
  }

  outColor = colors[closestValue - 1u];
}
