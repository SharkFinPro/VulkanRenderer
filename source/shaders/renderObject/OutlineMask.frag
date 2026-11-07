#version 450

layout(push_constant) uniform PushConstants {
  uint maskValue;
};

// Written by MousePicking.vert, which this pass shares; the mask has no use for it
layout(location = 0) flat in uint pickId;

layout(location = 0) out uint outMask;

void main()
{
  outMask = maskValue;
}
