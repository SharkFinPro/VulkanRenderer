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

  float r = (objectID >> 16) & 0xFF;
  float g = (objectID >> 8) & 0xFF;
  float b = (objectID >> 0) & 0xFF;

  outColor = uvec4(r, g, b, 255);
}
