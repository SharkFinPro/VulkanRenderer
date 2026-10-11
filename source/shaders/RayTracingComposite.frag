#version 450

layout(set = 0, binding = 0) uniform sampler2D tracedColor;
layout(set = 0, binding = 1) uniform sampler2D tracedDepth;

layout(location = 0) out vec4 outColor;

void main()
{
  ivec2 pixel = ivec2(gl_FragCoord.xy);

  outColor = vec4(texelFetch(tracedColor, pixel, 0).rgb, 1.0);
  gl_FragDepth = texelFetch(tracedDepth, pixel, 0).r;
}
