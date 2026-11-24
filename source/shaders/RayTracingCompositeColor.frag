#version 450

layout(set = 0, binding = 0) uniform sampler2D tracedColor;

layout(location = 0) out vec4 outColor;

// The color of RayTracingComposite.frag alone, for frames where nothing drawn after it tests depth
void main()
{
  outColor = vec4(texelFetch(tracedColor, ivec2(gl_FragCoord.xy), 0).rgb, 1.0);
}
