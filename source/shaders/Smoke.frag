#version 450

layout(location = 0) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

void main()
{
  vec2 coord = gl_PointCoord - vec2(0.5);
  float r = length(coord);
  float alpha = smoothstep(0.5, 0.4, r) * fragColor.a;

  vec2 noiseCoord = gl_PointCoord * 3.0;
  float noise = fract(sin(dot(noiseCoord, vec2(12.9898, 78.233))) * 43758.5453);
  alpha *= 0.8 + 0.2 * noise;

  outColor = vec4(fragColor.rgb, alpha);
}