#version 450
#extension GL_GOOGLE_include_directive : require
#include "common/Sky.glsl"

layout(location = 0) in vec2 ndc;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform SkyPushConstant {
  mat4 viewInverse;
  mat4 projInverse;
};

void main()
{
  // The same view direction the ray generation shader builds for this pixel
  vec4 target = projInverse * vec4(ndc, 1.0, 1.0);
  vec3 viewDirection = normalize(target.xyz / target.w);
  vec3 direction = (viewInverse * vec4(viewDirection, 0.0)).xyz;

  outColor = vec4(skyColor(direction), 1.0);
}
