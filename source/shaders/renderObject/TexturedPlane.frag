#version 450
#extension GL_GOOGLE_include_directive : require
#include "../common/AlphaCutoff.glsl"

layout(set = 1, binding = 0) uniform sampler2D texSampler;
layout(set = 1, binding = 1) uniform sampler2D specSampler;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
// Written by the shared vertex shader; only some fragment shaders need the object
layout(location = 3) flat in uint fragObject;

layout(location = 0) out vec4 outColor;

void main()
{
  const vec4 texColor = texture(texSampler, fragTexCoord);

  if (texColor.a < ALPHA_CUTOFF)
  {
    discard;
  }

  outColor = vec4(texColor.rgb, 1.0);
}
