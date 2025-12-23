#version 450

layout(push_constant) uniform GlyphPC {
  layout(offset = 108)
  float r;
  float g;
  float b;
  float a;
} pc;

layout(set = 0, binding = 0) uniform sampler2D glyphAtlas;

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

void main()
{
  float alpha = texture(glyphAtlas, fragUV).r;

  outColor = vec4(pc.r, pc.g, pc.b, alpha * pc.a);
}