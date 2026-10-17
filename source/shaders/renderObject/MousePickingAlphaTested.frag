#version 450
#extension GL_GOOGLE_include_directive : require
#include "../common/AlphaCutoff.glsl"

layout(set = 0, binding = 1) uniform sampler2D texSampler;

layout(push_constant) uniform PushConstants {
  uint objectID;
};

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out uvec4 outColor;

void main()
{
  if (texture(texSampler, fragTexCoord).a < ALPHA_CUTOFF)
  {
    discard;
  }

  // Same layout as MousePicking.frag: object, triangle within its draw, and the exact depth.
  outColor = uvec4(objectID, uint(gl_PrimitiveID), floatBitsToUint(gl_FragCoord.z), 0u);
}
