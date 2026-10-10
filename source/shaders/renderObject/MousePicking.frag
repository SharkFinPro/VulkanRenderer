#version 450

layout(push_constant) uniform PushConstants {
  uint objectID;
};

layout(location = 0) out uvec4 outColor;

void main()
{
  // Object (0 is the clear value, so ids start at 1), triangle within its draw, and the exact depth, so the CPU can
  // rebuild the world position.
  outColor = uvec4(objectID, uint(gl_PrimitiveID), floatBitsToUint(gl_FragCoord.z), 0u);
}
